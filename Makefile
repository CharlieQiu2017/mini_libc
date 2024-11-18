CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
FREEFLAGS = -nostdlib -ffreestanding
WARNFLAGS = -Wall -Wextra -pedantic -Werror -Wfatal-errors
ARCHFLAGS = -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
PROTFLAGS = -fomit-frame-pointer -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection
GCFLAGS = -ffunction-sections

ifeq ($(optimize),1)
OPTFLAGS = -Os -fweb
else
OPTFLAGS = -O0
endif

INCFLAGS += -I ./include
EXTFLAGS = 

CFLAGS = $(FREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(GCFLAGS) $(OPTFLAGS) $(INCFLAGS) $(EXTFLAGS)

LIBC_SRCS = $(shell find src -regex '.*\.c')
LIBC_OBJS = $(patsubst src/%.c,obj/%.o,$(LIBC_SRCS))
LIBC_LOBJS = $(patsubst src/%.c,obj/%.lo,$(LIBC_SRCS))
LIBC_OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(LIBC_OBJS))))

all: crt.o libc.a libc_pic.a

$(LIBC_OBJS) $(LIBC_LOBJS) : | $(LIBC_OBJ_DIRS)

$(LIBC_OBJ_DIRS) :
	mkdir -p $@

crt.o : crt/crt.asm
	$(AS) -o $@ $<

obj/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.lo : src/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libc.a : $(LIBC_OBJS)
	$(AR) rc $@ $^

libc_pic.a : $(LIBC_LOBJS)
	$(AR) rc $@ $^

clean :
	rm -rf obj/* crt.o libc.a libc_pic.a

.PHONY : all clean
