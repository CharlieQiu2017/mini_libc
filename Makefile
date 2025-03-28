CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
M4 = m4
FREEFLAGS = -nostdlib -ffreestanding
WARNFLAGS = -Wall -Wextra -pedantic -Werror -Wfatal-errors
ARCHFLAGS = -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
PROTFLAGS = -fomit-frame-pointer -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection
GCFLAGS = -ffunction-sections

ifeq ($(optimize),1)
OPTFLAGS = -Os -fweb
else
OPTFLAGS = -O0
ifeq ($(debug),1)
OPTFLAGS += -g -fdebug-prefix-map=..=$$(readlink -f ..)
endif
endif

INCFLAGS = -I ./include
EXTFLAGS = 

CFLAGS = $(FREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(GCFLAGS) $(OPTFLAGS) $(INCFLAGS) $(EXTFLAGS)

# Template sources
# These files must be first processed by M4
LIBC_TMPLS = $(shell find src -regex '.*\.m4')
LIBC_TMPL_SRCS = $(patsubst src/%.m4,tmp/%.c,$(LIBC_TMPLS))
LIBC_TMPL_DIRS = $(sort $(patsubst %/,%,$(dir $(LIBC_TMPL_SRCS))))

# Raw sources
# These files can be compiled directly
LIBC_SRCS = $(shell find src -regex '.*\.c')

# Object files
LIBC_OBJS = $(patsubst src/%.c,obj/%.o,$(LIBC_SRCS)) $(patsubst tmp/%.c,obj/%.o,$(LIBC_TMPL_SRCS))
LIBC_LOBJS = $(patsubst src/%.c,obj/%.lo,$(LIBC_SRCS)) $(patsubst tmp/%.c,obj/%.lo,$(LIBC_TMPL_SRCS))
LIBC_OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(LIBC_OBJS))))

all: crt.o libc.a libc_pic.a

archive:
	tar -C .. -czf ../mini_libc.tar.gz mini_libc

$(LIBC_OBJS) $(LIBC_LOBJS) : | $(LIBC_OBJ_DIRS)

$(LIBC_TMPL_SRCS) : | $(LIBC_TMPL_DIRS)

$(LIBC_OBJ_DIRS) $(LIBC_TMPL_DIRS) :
	mkdir -p $@

crt.o : crt/crt.asm
	$(AS) -o $@ $<

obj/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/%.c : src/%.m4
	$(M4) $< > $@

obj/%.o : tmp/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.lo : src/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

obj/%.lo : tmp/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libc.a : $(LIBC_OBJS)
	$(AR) rc $@ $^

libc_pic.a : $(LIBC_LOBJS)
	$(AR) rc $@ $^

clean :
	$(RM) -r obj tmp crt.o libc.a libc_pic.a

.PHONY : all clean archive
