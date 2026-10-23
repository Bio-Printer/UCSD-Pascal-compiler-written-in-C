# The compiler on the host (build/pc): gcc, 16-bit INTEGER, the P-machine's REAL emulated.
SRC = src/globals.c src/builtin.c src/sets.c src/heap.c src/preal.c src/procs.c \
      src/compinit.c src/decpart.c src/bodypart.c src/unitpart.c src/block.c \
      src/pcio_host.c src/pc_host.c
CFLAGS = -g -O1 -Wall -Wno-unused -Wno-unknown-pragmas -Wno-parentheses -Wno-maybe-uninitialized -I src

build/pc: $(SRC) src/*.h
	mkdir -p build
	gcc $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf build
