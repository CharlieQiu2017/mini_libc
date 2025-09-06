CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
LD = /opt/aarch64-none-elf/bin/aarch64-none-elf-ld
M4 = m4
STDFLAGS = -std=c11
FREEFLAGS = -nostdlib -ffreestanding
WARNFLAGS = -Wall -Wextra -pedantic -Werror -Wfatal-errors
ARCHFLAGS = -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
PROTFLAGS = -fomit-frame-pointer -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection
GCFLAGS = -ffunction-sections
LDFLAGS = -nostdlib -static --no-dynamic-linker -e _start --gc-sections --build-id=none -T default.lds

# If we choose to optimize the code, then we cannot debug it

ifeq ($(optimize),1)
  OPTFLAGS = -Os -fweb
else
  OPTFLAGS = -O0

  ifeq ($(debug),1)

# If we choose to debug the code, then optionally allow the user to pass in DEBUG_SRC_DIR,
# since we will likely debug the code on a different machine than the machine used to compile it.

    ifndef DEBUG_SRC_DIR
      OPTFLAGS += -g
    else
      OPTFLAGS += -g -fdebug-prefix-map=$$(pwd -L)=$(DEBUG_SRC_DIR)
    endif

  endif # ifeq($(debug),1)
endif # ifeq ($(optimize),1)

LIBGCC = /opt/aarch64-none-elf/lib/gcc/aarch64-none-elf/14.2.0/libgcc.a

INCFLAGS = -I ./include
EXTFLAGS = 

CFLAGS = $(STDFLAGS) $(FREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(GCFLAGS) $(OPTFLAGS) $(INCFLAGS) $(EXTFLAGS)

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

# Unit tests
LIBC_TEST_SRCS = $(shell find test-src -regex '.*\.c')
LIBC_TEST_OBJS = $(patsubst test-src/%.c,test-bin/%.o,$(LIBC_TEST_SRCS))
LIBC_TEST_BINS = $(patsubst test-src/%.c,test-bin/%,$(LIBC_TEST_SRCS))
LIBC_TEST_OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(LIBC_TEST_OBJS))))

all: crt.o libc.a libc_pic.a $(LIBC_TEST_BINS)

archive:
	tar -C .. -czf ../mini_libc.tar.gz mini_libc

$(LIBC_OBJS) $(LIBC_LOBJS) : | $(LIBC_OBJ_DIRS)

$(LIBC_TMPL_SRCS) : | $(LIBC_TMPL_DIRS)

$(LIBC_TEST_OBJS) $(LIBC_TEST_BINS) : | $(LIBC_TEST_OBJ_DIRS)

$(LIBC_OBJ_DIRS) $(LIBC_TMPL_DIRS) $(LIBC_TEST_OBJ_DIRS) :
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

test-bin/%.o : test-src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

libc.a : $(LIBC_OBJS)
	$(AR) rc $@ $^

libc_pic.a : $(LIBC_LOBJS)
	$(AR) rc $@ $^

test-bin/% : test-bin/%.o libc.a
	$(LD) $(LDFLAGS) -o $@ crt.o $^ $(LIBGCC)

clean :
	$(RM) -r obj test-bin tmp crt.o libc.a libc_pic.a

.PHONY : all clean archive
