
# {baby,mama,gran}-a-fallen-lap-ray DEFCON 2021 Quals

![a-fallen-lap-ray=parallel-af-yan](./images/name.gif "a-fallen-lap-ray=parallel-af-yan")

This challenge is an implementation of the
[pwn.college "Yan85" vm](https://pwn.college/modules/reversing)
written on top of the [Manchester parallel machine](https://github.com/o-o-overflow/dc2020f-parallel-af-public).

Authors: [adamd](https://twitter.com/adamdoupe) and
[Zardus](https://twitter.com/Zardus) (even though Zardus wrote zero
lines of code he insipred this challenge and acted as my rubber ducky
constantly during development.


## Challenge Conception/Idea

One idea for this challenge is that honestly I was bummed that teams
didn't really get to the depth of the
[parallel-af](https://github.com/o-o-overflow/dc2020f-parallel-af-public)
challenge, which had four bugs in phase 2 and a trap mode that was
never used. I felt that the teams really didn't understand the true
nature and cool-ness of the parallel machine, and the bugs that I had
did not exploit anything specific to the parallel machine architecure.

After chatting with Zardus, the core concept of the challenge
solidifed: the ["Yan85" vm](https://pwn.college/modules/reversing) was
a VM very similar to x86 expect for one crucial difference: because
instruction opcodes and operands were single-bit
[you can have instructions that have multiple opcodes](./service/src/vm.force#L641)
(i.e. one instruction that does add, load from memory, and a syscall).
We hypothesized that a Yan85-style VM on a parallel machine could have
race conditions in multiple-opcode instructions due to the inherent
non-determinism of the parallel machine (there is no concept of a
program counter in a dataflow machine because instructions execute
when the data is ready), and this was the core inspiration of the
challenge.

## Implementation

Changed a decent amount in the program, but the main new components are:

- [arrays.c](./service/src/arrays.c) implementation of a new module in
  the parallel machine for arrays. This was a major pain in the
  original parallel-af challenge because all data in the machine
  needed to be 8 bytes max, and it's very hard to do a VM like that.
  Designed the instructions inspired by Lisp `aref` operations.

- [vm.force](./service/src/vm.force) the implementation of the Yan85
  VM in the force language.

- [vm-assembler.py](./service/src/vm-assembler.py) a Yan85 VM assembler. 

- [p aka vuln.vm](./service/src/vuln.vm) is a "simple" menu-based log
  system. Due to the constraints of Yan85 machine, programming this is
  a major PITA (e.g., only computation instruction is `add`), so I
  took a lot of shortcuts such as my implementation of
  [`print_int`](./service/src/programs/vm/print_int.vm) which only
  works for 0 through 10 (but at least it's documentated?).

## Stages

There were three stages to this challenge `baby`, `mama`, and `gran`,
with the differences between the two being a 4 byte difference in
checks of the same name
([the string "mama" was changed to "flag" in `vm` for stage 2](./service/src/Makefile#L92)
and [the string "gran" was changed to "flag" in `manchester` for stage 3](./service/src/Makefile#L93)).

The idea was that checks to ensure that the teams could not open the
`flag` file were added at the `vm` and then in the parallel machine
itself (and the goal was to bypass those).

### Stage 1 `baby`: Vuln in `p`

The goal of stage 1 was for teams to understand the Yan85 vm, identify a vulnerability, and write Yan85 shellcode. 

The intended path for `baby` was an off-by-one error [where logs are stored in `p`](./service/src/programs/vm/vuln.vm#L895).

This allows you to change the [`max_num_logs`](./service/src/programs/vm/vuln.vm#L896) variable so that you can start writing more log entries.

You can create one more log entry and overwrite the [`name_len`](./service/src/programs/vm/vuln.vm#L901) variable, which is [used when deciding how many bytes to read onto the stack for the `name` of the logs](./service/src/programs/vm/vuln.vm#L219).

This way, the next log entry will overflow the saved IP on the stack and redirect control to your Yan85 shellcode.

[My shellcode](./service/src/programs/vm/shellcode.vm) for this vulnerability writes "flag" onto the stack (so that it's not dependant on the memory layout of the machine), calls open, then reads the flag and writes it out.

Another approach that I know of that teams used to solve this stage
was to overwrite the `max_num_logs` then keep creating logs until you
reached the stack. I think the next stages were not possible with this
approach due to the slowness (deployment had a hard 5 minute timeout).

### Stage 2 `mama`: Vuln in `vm`

To goal of stage 2 was for teams to understand the race conditions due
to non-determinism in the parallel machine, and use that race
condition to exploit a
[TOCTOU](https://en.wikipedia.org/wiki/Time-of-check_to_time-of-use)
vulnerability in
[the check in the `vm` of "flag"](https://github.com/o-o-overflow/dc2021q-a-fallen-lap-ray/blob/master/service/src/vm.force#L549).

While [every instruction interpretation](./service/src/vm.force#L641)
happened sequentially, there is no data dependency between
[the `OPEN` syscall](./service/src/vm.force#L529) and
[`READ` syscall](./service/src/vm.force#L481), so a Yan85 VM
instruction of `sys OPEN|READ d` would be nondeterministic and roughly
~2% of the time a `READ` from stdin into the same buffer as the `OPEN`
would hit in the middle of the TOCTOU. To get this working I had to
experimentally add and remove instructions
([hence the checks for only some files](https://github.com/o-o-overflow/dc2021q-a-fallen-lap-ray/blob/master/service/src/vm.force#L549))

[My YanVM code exploit](./service/src/programs/vm/stage_2_exploit.vm)
for this stage created the string "alag" in memory and
[called a concurrent `OPEN` and `READ` syscall](./service/src/programs/vm/stage_2_exploit.vm#L35),
and after would check if it could read a known FD.

[My python exploit](./interaction/exploit_2.py#L108) would then send a
bunch of "f"s which would be used to win the race and overwrite "alag"
into "flag" and bypass the check.

Oh, I also had to
[write a stage 0 shellcode](./service/src/programs/vm/read_in_shellcode.vm)
which would read in the race shellcode because the race shellcode was
too big to put on the stack.

Unfortunately, there was an unintended shortcut solution because
[the equality check](./service/src/vm.force#L549) for "flag" in the
underlying parallel machine operated on 8 bytes, and equality
comparison compared all 8 bytes while the
[underlying parallel machine `OPN` instruction](./service/src/input_module.c#L111)
used C-strings (null byte delimited) to read the flag.

Therefore, if you tried to `OPEN` a file from the Yan85 vm with
`flag\x00a` it would bypass the check and read the flag
`:crying_face_emoji:`.
[I implemented the missing check](./service/src/vm.force#L540) but did
not deploy (discussed later), but this check increases the success
rate of the race to ~75%.

### Stage 3 `gran`: Vuln in `manchester`

The goal of stage 3 was for the teams to exploit `manchester` using bugs in the array module.

First they had to
[win another data race in the `OPEN` syscall](./service/src/vm.force#L546)
which should make it impossible to open a file up for writing (the VM
zeros out the writing bytes if it detects them). If teams used the
shortcut solution for stage 2 they would have no idea how to proceed
from here.

The `quit` file is the only writable file in the whole directory (and
path traversal
[should be impossible due to a check](./service/src/input_module.c#L76)),
and the only modification to `sh` was to first call `vm p` then
[call `quit`](./service/src/sh.force#L120).

If you send up a Yan85 sys `OPEN|READ` syscall to try and open `quit`
and point the flags to the same memory location as `quit`, it will try
to open the file `puit` (due to the flag check) in read mode. However,
by using the idea of a data race and sending a `q` as input, if you
win the race you will be able to open `quit` for `writing`.

Now, you can write a parallel machine program!

The next part takes advantage of two vulnerabilities in the array
module (heap read and arbitrary write) and a data leak in the queue
module.

The
[`ARF` instruction has a signed/unsigned vulnerability that allows heap memory read](./service/src/arrays.c#L122)
which allows an attack from a parallel machine program to read data on
the heap from a negative location from a new array.

The
[`AST` instruction has an arbitrary write vulnerability](./service/src/arrays.c#L197)
because even though it is checking that the given point is in the
known array bounds the check is incorrect.

I'm not a heap expert, so I wasn't sure that using only these two
primitives if you could achieve code execution (you cannot trigger any
`free`s). Plus, the [seccomp filters](./service/src/arrays.c#L27) for
the array module would mean that you couldn't read the flag anyway
(we'll see how to get around that).

So, there was a bug in the queue module where
[the queue struct that was created on the stack](./service/src/queue.c#L50)
was [copied onto the heap](./service/src/queue.c#L144) and would leak
a stack address onto the heap.

At this point you can leak the stack address from the heap, then use
the arbitrary write to write values on to the stack. But what to
write? The
[array module executes as a giant while loop](./service/src/arrays.c#L56),
so I don't think you can overwrite saved rip on the stack.

My solution was to target the currently processing `execution_packet`
[on the stack](./service/src/arrays.c#L58), specifically
[to set the `ring` to zero, `opcode` to `OPN`, and `data_1` to `flag`](./service/src/types.h#L127),
which would bypass the
[stage 3 check in the input module](./service/src/input_module.c#L103).
(In full honesty, my original plan was to keep the opcodes and structs
of the parallel machine the same, but since I had to change the
`queue` struct and the `execution_packet` struct to accomplish this I
decided to modify all the rest of the machine so it wasn't obvious.)

This resulted in
[a parallel machine program](./service/src/programs/assembly/stage_3_exploit.tass)
that would first leak out the value from the heap, then
[call an `AST` instruction](./service/src/programs/assembly/stage_3_exploit.tass#L41)
that would actually transform itself into an `OPN flag` in ring 0
instruction.

Putting this all together into
[an exploit.py](./interaction/exploit_3.py) essentially first exploits
stage 1 vulnerability to execute the shellcode loaded.

Then it sends
[Yan85 shellcode to first win the data race to open `q` for writing, then reads in the `quit` parallel program.](./service/src/programs/vm/stage_3_exploit.vm)

This exploit works 1/16 times because the last byte of the stack
leakage is overwritten by the struct `ring` field. Since the parallel
machine is Turing complete I'm sure a sufficiently smart human could
write a deterministic exploit.

## Lessons Learned


### Syntax highlighting is golden

From a development perspective, nothing helped more than
[creating an emacs mode for the `force` language](./service/src/force-mode.el).
I didn't do this for Yan85 and it was a massive pain.

## Non-determinism is the enemy of a race

Long story short, my parallel machine compiler *and* assembler were
non-deterministic because I was using Python `set`s and
`defaultdict`s, and this would affect the generated code. This caused
hair-pullingly difficult problems where I would test the data race,
make a change to make it easier to win, and it would somehow be
harder. Note to self: always ensure your compiler is deterministic
(and `PYTHONHASHSEED=0` is your friend).

## Shortcut solutions on difficult challenges: what to do about them?

The stage 2 shortcut solution was very frustrating. Of course I tested
my stage 1 solution against stage 2 (it did not work), and I even
tested the first stage 1 solution from the teams against the
unreleased stage 2 (it also did not work). So we released stage 2, but
teams were exploiting it much quicker than I expected.

After digging in (while very sleep deprived), I realized what the bug was and we were faced with a few choices:

- Deploy a new fixed version of `mama` (stage 2) with the fix. Not really an
  option b/c with 3 stages this whole challenge direction is worth a
  lot of points already, and adding another is probably too much.

- Redeploy the fixed version of `mama` and take the flags away from
  people that already solved it. This isn't something that we've
  really done before, and by this point 3 or 4 teams had already
  solved it. They might have redirected people away from the problem
  once it was solved. Another risk is that the additional check
  changes the race condition and made it way easier (which would have
  required significant tweaking on my part, which is very risky to do
  with sleep deprivation). We decided against this option
  (although I still wonder if this would have been better).

- Do nothing. We reasoned that this shortcut solution of stage 2 would
  get them no closer to stage 3 (since they have no idea about the
  data race), so in this way stage 3 would sort of make up the
  difference. Ultimately this is what we decided to do.

- Release a fixed version of `mama` as a hint for stage 3, saying that
  if you solved the fixed version of `mama` then you were on the way
  to the first part of stage 3. I explored this option (creating the
  fix and ensuring that it was solvable), and polled the teams that
  were the first to solve stage 1 and stage 2. When I realized that
  they found the data race we abandoned this idea.

Still not sure what the right approach was here, but at least now you
understand what happened and our reasoning.



## Libc version differences

I did development and exploitation of the challenge on a remote
machine. Then, I built the challenge for deployment on my laptop and
pushed to infra. All should be fine I thought, the teams have complete
knowledge of the challenge: the exact Dockerfile and binaries,
everything to run the challenge exactly as it's running in the infra.
I even had the thought "I should probably give them the libc so", then
I thought what's the point they have full knowledge.

Unfortunately I didn't realize that my laptop's docker was using an
old cached version of `ubuntu:18.04` and the deployed `libc` version
was ~1 year old. This caused unexpected and unintended hardship for
teams that were very close to exploiting stage 3, and I'm sorry for
that.


## Reuse of old challenges, is it fair?

One of the things that drew me to this challenge is that parallel
machines represent a fundamentally different model of computation (a
computer architecture *without a program counter* and *the finest
grain parallelism possible*.) And teams in 2020 finals barely
scratched the surface of this cool concept.

Of course, teams that played DEFCON CTF 2020 finals would have an
advantage on such a challenge due to familiarity with the concept and
hands-on experience (although of all 16 teams PPP was the one that had
the best command of the parallel machine IIRC).

We discussed it internally and decided that the following factors were
in play: (1) the `parallel-af` machine was open-source since finals,
(2) I discussed the ideas and concepts of the parallel machine
publicly
([OOO CTF recaps](https://www.youtube.com/watch?v=LfyJzakCD9I)), and
(3) the race condition style-bug was not used at all in finals that it
would be OK. We would also point people to the parallel-af source code
so that they could start from there (and have code/docs/papers to
reference).

The feedback we received was that some teams still felt that this
unfairly benefited finals teams, which we know kind of agree with. One
thing to note is that I've heard from several teams that they
exploited stage 1 in a completely black-box way and did no real
reversing of the vm (if I understand them correctly), which is pretty
cool.

# The end

Hope you enjoyed the challenge, it was a joy and a pain to write, and
I learned a ton. I hope you did too.

`- adamd`


