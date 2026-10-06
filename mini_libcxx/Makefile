CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
CXX = /opt/aarch64-none-elf/bin/aarch64-none-elf-g++
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
LD = /opt/aarch64-none-elf/bin/aarch64-none-elf-ld
JINJA = jinja2
STDFLAGS = -std=c11
CXXSTDFLAGS = -std=c++26
FREEFLAGS = -nostdlib -ffreestanding
CXXFREEFLAGS = -nostdlib -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics
WARNFLAGS = -Wall -Wextra -pedantic -Werror -Wfatal-errors -Wno-error=array-bounds
ARCHFLAGS = -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
PROTFLAGS = -fomit-frame-pointer -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection -fno-ident -fno-jump-tables
PIEFLAGS = -fPIE
GCFLAGS = -fvisibility=hidden -ffunction-sections
LDFLAGS = -nostdlib --no-dynamic-linker -z max-page-size=4096 -e _start --gc-sections --build-id=none

ifeq ($(optimize),1)
  OPTFLAGS = -Os -fweb
else
  OPTFLAGS = -O0
  ifeq ($(debug),1)
    ifndef DEBUG_SRC_DIR
      OPTFLAGS += -g
    else
      OPTFLAGS += -g -fdebug-prefix-map=$$(pwd -L)=$(DEBUG_SRC_DIR)
    endif
  endif
endif

LIBGCC = /opt/aarch64-none-elf/lib/gcc/aarch64-none-elf/16.2.0/libgcc.a
LIBC = /home/z/mini_libc
LIBSUPCXX = /opt/aarch64-none-elf/aarch64-none-elf/lib/libsupc++.a

ifeq ($(pie),1)
  LDFLAGS += -shared -Bsymbolic -T $(LIBC)/default_pic.lds
else
  LDFLAGS += -static -T $(LIBC)/default.lds
endif

INCFLAGS = -I ./include -I $(LIBC)/include -I $(LIBC)/compat-include
EXTFLAGS = 

CFLAGS = $(STDFLAGS) $(FREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(PIEFLAGS) $(GCFLAGS) $(OPTFLAGS) $(INCFLAGS) $(EXTFLAGS)
CXXFLAGS = $(CXXSTDFLAGS) $(CXXFREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(PIEFLAGS) $(GCFLAGS) $(OPTFLAGS) $(INCFLAGS) $(EXTFLAGS)

# Template sources
# These files must be first processed by Jinja2
C_J2_TMPLS = $(shell find src -regex '.*\.c.j2')
C_J2_TMPL_SRCS = $(patsubst src/%.j2,tmp/%,$(C_J2_TMPLS))
C_J2_TMPL_DIRS = $(sort $(patsubst %/,%,$(dir $(C_J2_TMPL_SRCS))))

CXX_J2_TMPLS = $(shell find src -regex '.*\.cpp.j2')
CXX_J2_TMPL_SRCS = $(patsubst src/%.j2,tmp/%,$(CXX_J2_TMPLS))
CXX_J2_TMPL_DIRS = $(sort $(patsubst %/,%,$(dir $(CXX_J2_TMPL_SRCS))))

# Raw sources
# These files can be compiled directly
C_SRCS = $(shell find src -regex '.*\.c')
CXX_SRCS = $(shell find src -regex '.*\.cpp')

# Object files
OBJS = $(patsubst src/%.c,obj/%.o,$(C_SRCS)) $(patsubst tmp/%.c,obj/%.o,$(C_J2_TMPL_SRCS)) $(patsubst src/%.cpp,obj/%.o,$(CXX_SRCS)) $(patsubst tmp/%.cpp,obj/%.o,$(CXX_J2_TMPL_SRCS))
OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(OBJS))))

# Unit tests
C_TEST_SRCS = $(shell find test-src -regex '.*\.c')
CXX_TEST_SRCS = $(shell find test-src -regex '.*\.cpp')
TEST_OBJS = $(patsubst test-src/%.c,test-bin/%.o,$(C_TEST_SRCS)) $(patsubst test-src/%.cpp,test-bin/%.o,$(CXX_TEST_SRCS))
TEST_BINS = $(patsubst test-src/%.c,test-bin/%,$(C_TEST_SRCS)) $(patsubst test-src/%.cpp,test-bin/%,$(CXX_TEST_SRCS))
TEST_OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(TEST_OBJS))))

all: libcxx.a $(TEST_BINS)

archive:
	tar -C .. -czf ../mini_libcxx.tar.gz mini_libcxx

$(OBJS) : | $(OBJ_DIRS)

$(C_J2_TMPL_SRCS) $(CXX_J2_TMPL_SRCS) : | $(C_J2_TMPL_DIRS) $(CXX_J2_TMPL_DIS)

$(TEST_OBJS) $(TEST_BINS) : | $(TEST_OBJ_DIRS)

$(OBJ_DIRS) $(C_J2_TMPL_DIRS) $(CXX_J2_TMPL_DIRS) $(TEST_OBJ_DIRS) :
	mkdir -p $@

obj/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o : src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

tmp/%.c : src/%.c.j2 src/%.c.json
	$(JINJA) $^ > $@

tmp/%.cpp : src/%.cpp.j2 src/%.cpp.json
	$(JINJA) $^ > $@

obj/%.o : tmp/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o : tmp/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-bin/%.o : test-src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test-bin/%.o : test-src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

libcxx.a : $(OBJS)
	$(AR) rc $@ $^

test-bin/% : test-bin/%.o libcxx.a
	$(LD) $(LDFLAGS) -o $@ $(LIBC)/crt.o $^ $(LIBSUPCXX) $(LIBC)/libc.a $(LIBGCC)

clean :
	$(RM) -rf obj test-bin tmp libcxx.a

.PHONY : all clean archive
