# The compiler on the host (build/pc): gcc, 16-bit INTEGER, the P-machine's REAL emulated.
SRC = src/globals.c src/builtin.c src/sets.c src/heap.c src/preal.c src/procs.c \
      src/compinit.c src/decpart.c src/bodypart.c src/unitpart.c src/block.c src/rare.c \
      src/pcio_host.c src/pc_host.c
CFLAGS = -g -O1 -Wall -Wno-unused -Wno-unknown-pragmas -Wno-parentheses -Wno-maybe-uninitialized -I src

build/pc: $(SRC) src/*.h
	mkdir -p build
	gcc $(CFLAGS) -o $@ $(SRC)

.PHONY: psys clean

clean:
	rm -rf build

# The compiler on the P-System (build/psys/PC.CODE): UCSD-TinyC's host Tiny-C beside this repository.
TINYC ?= ../ucsd-tinyc
PSRC = src/globals.c src/builtin.c src/sets.c src/heap.c src/procs.c \
       src/compinit.c src/decpart.c src/bodypart.c src/unitpart.c src/block.c src/rare.c \
       src/pcio_psys.c src/pc_psys.c

build/psys/PC.CODE: $(PSRC) src/*.h
	mkdir -p build/psys
	$(TINYC)/build/tc -I $(TINYC)/tinyc/include -o $@ $(PSRC)

psys: build/psys/PC.CODE
