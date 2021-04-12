# VM language calling convention

`a` register can be clobbered

`b`, `c`, and `d` are callee saved

Stack is:

```
arg[n]
arg[n-1]
...
arg[0]
saved_ip
```
