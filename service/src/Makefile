# Source, Executable, Includes, Library Defines
INCL   = $(wildcard *.h)
SRC    = $(filter-out schitzo.c,$(wildcard *.c))
OBJ    = $(SRC:%.c=%.o)

PLATFORM = $(shell uname)
LIBS   = -pthread

BUILDDIR = build

FORCE = $(wildcard *.force)
PROGS = $(FORCE:%.force=$(BUILDDIR)/%)
VM_PROGS = $(wildcard programs/vm/*.vm)

# If the platform is not darwin (Mac), the use the -lrt flag (needed on linux)
ifneq (,$(findstring Darwin,$(PLATFORM)))
else
	LIBS := $(LIBS) -lrt
endif

EXE    = $(BUILDDIR)/manchester

# Compiler, Linker Defines
CC      = gcc
CFLAGS  = -std=gnu11 -Wall -O2
LIBPATH = -L.
LDFLAGS = -o $(EXE) $(LIBPATH) $(LIBS)
CFDEBUG = -std=gnu11 -Wall -g -DDEBUG $(LDFLAGS)
RM      = /bin/rm -f

.PHONY: clean deploy debug

# Compile and Assemble C Source Files into Object Files
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $@

# Link all Object Files with external Libraries into Binaries
$(EXE): $(OBJ) $(PROGS)
	$(CC) $(OBJ) $(LDFLAGS)

schitzo: schitzo.c
	$(CC) schitzo.c -o schitzo

# Objects depend on these Libraries
$(OBJ): $(INCL)

$(PROGS): $(FORCE) compiler.py assembler.py
	python compiler.py --file $(notdir $@).force --assembly $(notdir $@).tass --output $@

build/p: $(VM_PROGS) vm-assembler.py
	python vm-assembler.py --file ./programs/vm/vuln.vm --output ./build/p

# Create a gdb/dbx Capable Executable with DEBUG flags turned on
debug: $(PROGS)
	$(CC) $(SRC) $(CFDEBUG)

# Clean Up Objects, Exectuables, Dumps out of source directory
clean:
	$(RM) $(OBJ) $(EXE) core a.out $(PROGS)

deploy: $(EXE) $(PROGS) schitzo build/p
	strip $(BUILDDIR)/manchester
	cp $(BUILDDIR)/manchester ../manchester

	python strip.py --file $(PROGS)

	cp $(PROGS) ../
	cp build/p ../
	cp schitzo ../
	chmod 666 ../quit
	rm -f ../parallel-af.tar.gz || true
	cd ../ && tar -zcvf a-fallen-lap-ray.tar.gz manchester os sh quit vm && cd -

