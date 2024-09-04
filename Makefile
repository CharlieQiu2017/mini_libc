CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
CFLAGS = -nostdinc -nostdlib -ffreestanding
CFLAGS += -Wall -Wextra -pedantic -Werror -Wfatal-errors
CFLAGS += -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
CFLAGS += -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection
CFLAGS += -ffunction-sections

ifeq ($(optimize),1)
CFLAGS+=-Os
else
CFLAGS+=-O0
endif

CFLAGS += -I ./include

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
