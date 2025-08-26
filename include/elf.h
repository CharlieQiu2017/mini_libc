/* Definitions related to AArch64 Linux ELF format
   Adapted from:
   https://refspecs.linuxbase.org/elf/gabi4+/contents.html
   https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst
   binutils include/elf/internal.h
   binutils include/elf/common.h
   binutils include/elf/aarch64.h
 */

#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef int32_t Elf64_Sword;
typedef int64_t Elf64_Sxword;

/* ELF header */

struct Elf64_Ehdr {
  unsigned char e_ident[16];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry; /* Entry-point address */
  Elf64_Off e_phoff; /* Program header offset */
  Elf64_Off e_shoff; /* Section header offset */
  Elf64_Word e_flags; /* Should be 0 */
  Elf64_Half e_ehsize; /* Size of ELF header, should be 0x40 */
  Elf64_Half e_phentsize; /* Size of each program header entry */
  Elf64_Half e_phnum; /* Number of program headers */
  Elf64_Half e_shentsize; /* Size of each section header entry */
  Elf64_Half e_shnum; /* Number of section headers */
  Elf64_Half e_shstrndx; /* Section header table index of the section name string table */
};

/* e_ident fields */

#define EI_MAG0 0 /* Should be 0x7f */
#define EI_MAG1 1 /* Should be 'E' */
#define EI_MAG2 2 /* Should be 'L' */
#define EI_MAG3 3 /* Should be 'F' */
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8 /* For Linux, should be 0 */
#define EI_PAD 9 /* Reserved, should be 0 */
#define EI_NIDENT 16

/* EI_CLASS, should be always ELFCLASS64 */

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

/* EI_DATA, should be always ELFDATA2LSB */

#define ELFDATANONE 0
#define ELFDATA2LSB 1 /* little-endian */
#define ELFDATA2MSB 2 /* big-endian */

/* EI_OSABI
   AArch64 ELF Specification says:
   This field shall be zero unless the file uses objects
   that have flags which have OS-specific meanings.
 */

#define ELFOSABI_NONE 0
#define ELFOSABI_LINUX 3

/* e_type */

#define ET_NONE 0
#define ET_REL 1 /* Relocatable file */
#define ET_EXEC 2 /* Executable file */
#define ET_DYN 3 /* Dynamic library */
#define ET_CORE 4 /* Core dump */
#define ET_LOOS 0xfe00
#define ET_HIOS 0xfeff
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

/* e_machine, should be always EM_AARCH64 */

#define EM_AARCH64 0xb7

/* e_version, should be always EV_CURRENT */

#define EV_NONE 0
#define EV_CURRENT 1

/* Note on e_shnum field:
   If the number of sections is greater than or equal to SHN_LORESERVE (0xff00),
   e_shnum has the value SHN_UNDEF (0) and the actual number of section header
   table entries is contained in the sh_size field of the section header at index 0
   (otherwise, the sh_size member of the initial entry contains 0). 
 */

/* Program header */

struct Elf64_Phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset; /* Offset from beginning of file */
  Elf64_Addr p_vaddr; /* Start of virtual address */
  Elf64_Addr p_paddr; /* Physical address, unused */
  Elf64_Xword p_filesz; /* Size in file */
  Elf64_Xword p_memsz; /* Size in memory */
  Elf64_Xword p_align; /* Segment memory address alignment, usually page size */
};

/* p_type */

#define PT_NULL	0
#define PT_LOAD	1 /* Loadable segment */
#define PT_DYNAMIC 2 /* Dynamic section */
#define PT_INTERP 3 /* Interpreter string */
#define PT_NOTE 4 /* Note section */
#define PT_SHLIB 5 /* Reserved */
#define PT_PHDR	6 /* This array element, if present, specifices the location and size of the program header table itself */
#define PT_TLS 7 /* Thread-local storage template */
#define PT_LOOS	0x60000000
#define PT_HIOS	0x6fffffff
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

/* Reserved for architecture compatibility information */
#define PT_AARCH64_ARCHEXT 0x70000000
/* Reserved for exception unwinding tables */
#define PT_AARCH64_UNWIND 0x70000001
/* Reserved for MTE memory tag data dumps in core files */
#define PT_AARCH64_MEMTAG_MTE 0x70000002

/* p_flags */

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4
#define PF_MASKOS 0x0ff00000
#define PF_MASKPROC 0xf0000000

/* Section header */

struct Elf64_Shdr {
  Elf64_Word sh_name; /* Index into string table */
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr; /* Virtual memory address */
  Elf64_Off sh_offset; /* Offset from beginning of file */
  Elf64_Xword sh_size; /* Size in bytes */
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign; /* Alignment requirement of section */
  Elf64_Xword sh_entsize; /* For array sections like symbol table: size of each entry */
};

/* Special section indexes */

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_LOOS 0xff20
#define SHN_HIOS 0xff3f
/* This value specifies absolute values for the corresponding reference.
   For example, symbols defined relative to section number SHN_ABS
   have absolute values and are not affected by relocation.
 */
#define SHN_ABS 0xfff1
/* Symbols defined relative to this section are common symbols,
   such as FORTRAN COMMON or unallocated C external variables.
 */
#define SHN_COMMON 0xfff2
/* This value is an escape value.
   It indicates that the actual section header index is too large
   to fit in the containing field and is to be found in another location
   (specific to the structure where it appears).
 */
#define SHN_XINDEX 0xffff
#define SHN_HIRESERVE 0xffff

/* sh_type */

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_INIT_ARRAY 14
#define SHT_FINI_ARRAY 15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP 17
#define SHT_SYMTAB_SHNDX 18
#define SHT_RELR 19

#define SHT_LOOS 0x60000000
#define SHT_HIOS 0x6fffffff

#define SHT_GNU_HASH 0x6ffffff6

#define SHT_GNU_verdef 0x6ffffffd
#define SHT_GNU_verneed 0x6ffffffe
#define SHT_GNU_versym 0x6fffffff

#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

/* sh_flags */

#define SHF_WRITE (1 << 0)
#define SHF_ALLOC (1 << 1)
#define SHF_EXECINSTR (1 << 2)
#define SHF_MERGE (1 << 4)
#define SHF_STRINGS (1 << 5)
#define SHF_INFO_LINK (1 << 6)
#define SHF_LINK_ORDER (1 << 7)
#define SHF_OS_NONCONFORMING (1 << 8)
#define SHF_GROUP (1 << 9)
#define SHF_TLS (1 << 10)

#define SHF_MASKOS 0x0ff00000
#define SHF_MASKPROC 0xf0000000

/* Dynamic section entry */

struct Elf64_Dyn {
  Elf64_Sxword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr d_ptr;
  };
};

/* d_tag */

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTREL 20
#define DT_DEBUG 21
#define DT_TEXTREL 22
#define DT_JMPREL 23
#define DT_BIND_NOW 24
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_RUNPATH 29
#define DT_FLAGS 30
#define DT_PREINIT_ARRAY 32
#define DT_PREINIT_ARRAYSZ 33
#define DT_SYMTAB_SHNDX 34
#define DT_RELRSZ 35
#define DT_RELR 36
#define DT_RELRENT 37
#define DT_LOOS 0x6000000d
#define DT_HIOS 0x6ffff000
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff

#define DT_GNU_HASH 0x6ffffef5

struct Elf64_Sym {
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Half st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
};

/* st_info */

#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0x0f)
#define ELF64_ST_INFO(b,t) (((b) << 4) + ((t) & 0x0f))

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_COMMON 5
#define STT_TLS 6
#define STT_RELC 8
#define STT_SRELC 9
#define STT_LOOS 10
#define STT_GNU_IFUNC 10
#define STT_HIOS 12
#define STT_LOPROC 13
#define STT_HIPROC 15

/* st_other */

#define ELF64_ST_VISIBILITY(o) ((o) & 0x03)

#define STV_DEFAULT 0
#define STV_INTERNAL 1
#define STV_HIDDEN 2
#define STV_PROTECTED 3

#endif
