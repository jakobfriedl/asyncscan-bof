PROJECT = asyncscan
SRCS    := src/main.c
CCX64   := x86_64-w64-mingw32-gcc
CCX86   := i686-w64-mingw32-gcc
CFLAGS  := -Wall -Werror -Os -s -Iinclude -D_NO_NTDLL_CRT_

.DEFAULT: all
all: bof
bof: $(PROJECT).x64.o $(PROJECT).x86.o

$(PROJECT).x64.o: $(SRCS)
	$(CCX64) -c src/main.c -o dist/$(PROJECT).x64.o $(CFLAGS)

$(PROJECT).x86.o: $(SRCS)
	$(CCX86) -c src/main.c -o dist/$(PROJECT).x86.o $(CFLAGS)

clean:
	rm -f dist/$(PROJECT).x64.o dist/$(PROJECT).x86.o