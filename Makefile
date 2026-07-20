PROJECT  = asyncscan
CCX64    := x86_64-w64-mingw32-gcc
CCX86    := i686-w64-mingw32-gcc
CFLAGS   := -Wall -Werror -Os -s -Iinclude -D_NO_NTDLL_CRT_

.DEFAULT: all
all: bof
bof: portscan pingsweep

portscan: dist/portscan.x64.o dist/portscan.x86.o
pingsweep: dist/pingsweep.x64.o dist/pingsweep.x86.o

dist/portscan.x64.o: src/portscan.c src/common.c
	$(CCX64) -c src/portscan.c -o $@ $(CFLAGS)

dist/portscan.x86.o: src/portscan.c src/common.c
	$(CCX86) -c src/portscan.c -o $@ $(CFLAGS)

dist/pingsweep.x64.o: src/pingsweep.c src/common.c
	$(CCX64) -c src/pingsweep.c -o $@ $(CFLAGS)

dist/pingsweep.x86.o: src/pingsweep.c src/common.c
	$(CCX86) -c src/pingsweep.c -o $@ $(CFLAGS)

clean:
	rm -f dist/portscan.x64.o dist/portscan.x86.o dist/pingsweep.x64.o dist/pingsweep.x86.o