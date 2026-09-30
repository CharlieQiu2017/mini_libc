CC = /opt/aarch64-none-elf/bin/aarch64-none-elf-gcc
CXX = /opt/aarch64-none-elf/bin/aarch64-none-elf-g++
AS = /opt/aarch64-none-elf/bin/aarch64-none-elf-as
AR = /opt/aarch64-none-elf/bin/aarch64-none-elf-ar
STDFLAGS = -std=c11
CXXSTDFLAGS = -std=c++26
FREEFLAGS = -nostdlib -ffreestanding
CXXFREEFLAGS = -nostdlib -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics
WARNFLAGS = -Wall -Wextra -pedantic -Werror -Wfatal-errors
ARCHFLAGS = -march=armv8-a+crc+crypto -mtune=cortex-a72.cortex-a53
PROTFLAGS = -fomit-frame-pointer -fno-asynchronous-unwind-tables -fcf-protection=none -fno-stack-protector -fno-stack-clash-protection -fno-ident -fno-jump-tables
PIEFLAGS = -fPIE
GCFLAGS = -fvisibility=hidden -ffunction-sections
OPTFLAGS = -Os -fweb
EXTFLAGS = 
CFLAGS = $(STDFLAGS) $(FREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(PIEFLAGS) $(GCFLAGS) $(OPTFLAGS) $(EXTFLAGS)
CXXFLAGS = $(CXXSTDFLAGS) $(CXXFREEFLAGS) $(WARNFLAGS) $(ARCHFLAGS) $(PROTFLAGS) $(PIEFLAGS) $(GCFLAGS) $(OPTFLAGS) $(EXTFLAGS)

all: libsupc++.a

libsupc++.a: new.o terminate.o
	$(AR) rc $@ $^

clean:
	rm -f *.o *.a

.PHONY: all clean
