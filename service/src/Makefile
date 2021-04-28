# Source, Executable, Includes, Library Defines
INCL   = $(wildcard *.h)
SRC    = $(filter-out schitzo.c,$(wildcard *.c))
OBJ    = $(SRC:%.c=%.o)

PLATFORM = $(shell uname)
LIBS   = -pthread -lseccomp

BUILDDIR = build

FORCE = $(wildcard *.force)
PROGS = $(FORCE:%.force=$(BUILDDIR)/%)
VM_PROGS = $(wildcard programs/vm/*.vm)
DEPLOY_VM_PROGS = build/p build/shellcode build/shellcode2 build/read_in_shellcode

# If the platform is not darwin (Mac), the use the -lrt flag (needed on linux)
ifneq (,$(findstring Darwin,$(PLATFORM)))
else
	LIBS := $(LIBS) -lrt
endif

EXE    = $(BUILDDIR)/manchester

# Compiler, Linker Defines
CC      = gcc
CFLAGS  = -std=gnu11 -Wall -O2 -fshort-enums
LIBPATH = -L. 
LDFLAGS = -o $(EXE) $(LIBPATH) $(LIBS)
CFDEBUG = -std=gnu11 -Wall -g -DDEBUG -fshort-enums -O2 $(LDFLAGS)
RM      = /bin/rm -f

.PHONY: clean deploy debug

# Compile and Assemble C Source Files into Object Files
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $@

# Link all Object Files with external Libraries into Binaries
$(EXE): $(OBJ) $(PROGS) $(DEPLOY_VM_PROGS)
	$(CC) $(OBJ) $(LDFLAGS)

schitzo: schitzo.c
	$(CC) schitzo.c -o schitzo

# Objects depend on these Libraries
$(OBJ): $(INCL)

$(PROGS): $(FORCE) compiler.py assembler.py
	PYTHONHASHSEED=0 python compiler.py --file $(notdir $@).force --assembly $(notdir $@).tass --output $@

build/p: $(VM_PROGS) vm-assembler.py
	PYTHONHASHSEED=0 python vm-assembler.py --file ./programs/vm/vuln.vm --output ./build/p

build/shellcode: $(VM_PROGS) ./programs/vm/shellcode.vm
	PYTHONHASHSEED=0 python vm-assembler.py --file ./programs/vm/shellcode.vm --output ./build/shellcode

build/shellcode2: $(VM_PROGS) ./programs/vm/stage_2_exploit.vm
	PYTHONHASHSEED=0 python vm-assembler.py --file ./programs/vm/stage_2_exploit.vm --output ./build/shellcode2

build/read_in_shellcode: $(VM_PROGS) ./programs/vm/read_in_shellcode.vm
	PYTHONHASHSEED=0 python vm-assembler.py --file ./programs/vm/read_in_shellcode.vm --output ./build/read_in_shellcode


# Create a gdb/dbx Capable Executable with DEBUG flags turned on
debug: $(PROGS)
	$(CC) $(SRC) $(CFDEBUG)

# Clean Up Objects, Exectuables, Dumps out of source directory
clean:
	$(RM) $(OBJ) $(EXE) core a.out $(PROGS) $(DEPLOY_VM_PROGS)

deploy: $(EXE) $(PROGS) schitzo $(DEPLOY_VM_PROGS)
	strip $(BUILDDIR)/manchester
	cp $(BUILDDIR)/manchester ../manchester

	PYTHONHASHSEED=0 python strip.py --file $(PROGS)

	cp $(PROGS) ../
	cp build/p ../
	cp build/shellcode ../../interaction/
	cp build/shellcode2 ../../interaction/
	cp build/read_in_shellcode ../../interaction/
	cp schitzo ../
	chmod 666 ../quit
	rm -f ../parallel-af.tar.gz || true
	cd ../ && tar -zcvf a-fallen-lap-ray.tar.gz manchester os sh quit vm && cd -

