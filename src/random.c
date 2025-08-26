#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include <syscall_nr.h>
#include <auxv.h>
#include <elf.h>
#include <tls.h>
#include <memory.h>
#include <random.h>

typedef ssize_t (* getrandom_func) (void * buffer, size_t len, unsigned int flags, void * opaque_state, size_t opaque_len);

static getrandom_func getrandom_func_ptr = NULL;
static struct vgetrandom_opaque_params getrandom_params;
static void * getrandom_page;

void find_vdso_getrandom (void * auxv) {
  uint64_t * auxv_ptr = auxv;

  while (*auxv_ptr != AT_NULL) {
    if (*auxv_ptr != AT_SYSINFO_EHDR) { auxv_ptr += 2; continue; }

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

    /* Now, iterate through the symbol table to find the getrandom function */
    /* This is inefficient, we are essentially ignoring the hash-table.
       However, insofar as vDSO has only a few symbols, this is not a problem.
     */
    struct Elf64_Sym * sym_ent = symtab;

    for (i = 0; i < num_symbols; ++i) {
      if (ELF64_ST_TYPE (sym_ent->st_info) != STT_FUNC) { sym_ent++; continue; }
      uint32_t stridx = sym_ent->st_name;
      const char * str = ((const char *) strtab) + stridx;
      if (strcmp (str, "__kernel_getrandom") != 0) { sym_ent++; continue; }

      getrandom_func_ptr = (getrandom_func)(((uintptr_t) ehdr) + sym_ent->st_value);

      /* Get the opaque params */
      getrandom_func_ptr (0, 0, 0, &getrandom_params, ~0ull);

      /* Allocate opaque state space.
	 We shall assume that one page is enough for all threads.
	 TODO: Implement more robust framework for per-thread state management.
      */
      getrandom_page = mmap (NULL, 4096, getrandom_params.mmap_prot, getrandom_params.mmap_flags | MAP_ANONYMOUS, 0, 0);
      if (getrandom_page == NULL) getrandom_func_ptr = NULL;

      return;
    }

    return;
  }

  return;
}

ssize_t getrandom_syscall (void * buf, size_t buflen, unsigned int flags) {
  return syscall3 ((long) buf, buflen, flags, __NR_getrandom);
}

ssize_t getrandom (void * buf, size_t buflen, unsigned int flags) {
  if (getrandom_func_ptr) {
    uint16_t tid = get_thread_id ();
    return getrandom_func_ptr (buf, buflen, flags, ((char *) getrandom_page) + getrandom_params.size_of_opaque_state * tid, getrandom_params.size_of_opaque_state);
  }

  return getrandom_syscall (buf, buflen, flags);
}
