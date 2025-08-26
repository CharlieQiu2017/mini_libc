#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <auxv.h>
#include <elf.h>
#include <vdso.h>

static void * vdso_getrandom_ptr = NULL;
static void * vdso_gettimeofday_ptr = NULL;
static void * vdso_clock_gettime_ptr = NULL;
static void * vdso_clock_getres_ptr = NULL;

void interpret_vdso_from_auxv (void * auxv) {
  uint64_t * auxv_ptr = auxv;

  while (*auxv_ptr != AT_NULL && *auxv_ptr != AT_SYSINFO_EHDR) auxv_ptr += 2;
  if (*auxv_ptr == AT_NULL) return;

  uint32_t i;

  /* Find the ELF header address */
  struct Elf64_Ehdr * ehdr = (struct Elf64_Ehdr *)(*(auxv_ptr + 1));

  /* Find the Dynamic segment header */
  struct Elf64_Phdr * phdr = (struct Elf64_Phdr *)(((uintptr_t) ehdr) + ehdr->e_phoff);
  uint32_t phnum = ehdr->e_phnum;

  i = 0;
  while (i < phnum && phdr->p_type != PT_DYNAMIC) {
    /* We assume each program header is of size 0x38 */
    phdr = (struct Elf64_Phdr *)(((uintptr_t) phdr) + 0x38);
    i++;
  }

  if (i == phnum) return;

  /* Find the Dynamic segment */
  struct Elf64_Dyn * dyn_ent = (struct Elf64_Dyn *)(((uintptr_t) ehdr) + phdr->p_offset);

  void * strtab = NULL, * symtab = NULL, * hash = NULL, * gnuhash = NULL;

  while (dyn_ent->d_tag != DT_NULL) {
    if (dyn_ent->d_tag == DT_STRTAB) {
      strtab = (void *)(((uintptr_t) ehdr) + dyn_ent->d_ptr);
    }

    if (dyn_ent->d_tag == DT_SYMTAB) {
      symtab = (void *)(((uintptr_t) ehdr) + dyn_ent->d_ptr);
    }

    if (dyn_ent->d_tag == DT_HASH) {
      hash = (void *)(((uintptr_t) ehdr) + dyn_ent->d_ptr);
    }

    if (dyn_ent->d_tag == DT_GNU_HASH) {
      gnuhash = (void *)(((uintptr_t) ehdr) + dyn_ent->d_ptr);
    }

    dyn_ent++;
  }

  if (strtab == NULL || symtab == NULL || (hash == NULL && gnuhash == NULL)) return;

  uint32_t num_symbols = 0;

  if (hash != NULL) {

    num_symbols = *(uint32_t *)(((uintptr_t) hash) + 4);

  } else {

    /* Compute total number of symbols from the GNU hash table */
    /* The GNU hash table is poorly documented.
       See the source code of readelf, specifically the get_num_dynamic_syms() function */
    /* See also https://flapenguin.me/elf-dt-gnu-hash */

    /* We don't need the bloom filter, ignore it */
    uint32_t nbuckets, symoffset, bloom_size;
    nbuckets = *((uint32_t *) gnuhash);
    symoffset = *(uint32_t *)(((uintptr_t) gnuhash) + 4);
    bloom_size = *(uint32_t *)(((uintptr_t) gnuhash) + 8);

    uint32_t * buckets_ptr = (uint32_t *)(((uintptr_t) gnuhash) + 16 + 8 * bloom_size);
    uint32_t * chain_ptr = (uint32_t *)(((uintptr_t) gnuhash) + 16 + 8 * bloom_size + 4 * nbuckets);

    /* Find the highest non-empty bucket */
    uint32_t high_bucket = 0xffffffff;

    for (i = 0; i < nbuckets; ++i) {
      if (buckets_ptr[i] != 0) high_bucket = i;
    }

    if (high_bucket == 0xffffffff) return;

    uint32_t * chain_ptr_last = chain_ptr + (buckets_ptr[high_bucket] - symoffset);
    while (! (*chain_ptr_last & 1)) { ++chain_ptr_last; }

    num_symbols = (chain_ptr_last - chain_ptr) + 1 + symoffset;

  }

  /* Iterate through the symbol table to find the vDSO functions. */
  /* This is inefficient, we are essentially ignoring the hash-table.
     However, insofar as vDSO has only a few symbols, this is not a problem.
  */

  struct Elf64_Sym * sym_ent = symtab;

  for (i = 0; i < num_symbols; ++i) {
    if (ELF64_ST_TYPE (sym_ent->st_info) != STT_FUNC) { sym_ent++; continue; }
    uint32_t stridx = sym_ent->st_name;
    const char * str = ((const char *) strtab) + stridx;

    if (strcmp (str, "__kernel_getrandom") == 0) vdso_getrandom_ptr = (void *)(((uintptr_t) ehdr) + sym_ent->st_value);
    if (strcmp (str, "__kernel_gettimeofday") == 0) vdso_gettimeofday_ptr = (void *)(((uintptr_t) ehdr) + sym_ent->st_value);
    if (strcmp (str, "__kernel_clock_gettime") == 0) vdso_clock_gettime_ptr = (void *)(((uintptr_t) ehdr) + sym_ent->st_value);
    if (strcmp (str, "__kernel_clock_getres") == 0) vdso_clock_getres_ptr = (void *)(((uintptr_t) ehdr) + sym_ent->st_value);

    sym_ent++;
  }

  return;
}

void * get_vdso_getrandom_ptr (void) {
  return vdso_getrandom_ptr;
}

void * get_vdso_gettimeofday_ptr (void) {
  return vdso_gettimeofday_ptr;
}

void * get_vdso_clock_gettime_ptr (void) {
  return vdso_clock_gettime_ptr;
}

void * get_vdso_clock_getres_ptr (void) {
  return vdso_clock_getres_ptr;
}
