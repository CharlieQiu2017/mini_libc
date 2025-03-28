#ifndef SPHINCS_PARAMS_COMMON_H
#define SPHINCS_PARAMS_COMMON_H

/* SPHINCS+ depends on six parameters denoted by N, H, D, T, K, and W.
   However, W is set to 16 in all recommended instantiations.
   T is a power of 2 and we denote its log2 value by A.

   The other parameters should be set to:
        N  H  D  A  K
   128s 16 63 7  12 14
   128f 16 66 22 6  33
   192s 24 63 7  14 17
   192f 24 66 22 8  33
   256s 32 64 8  14 22
   256f 32 68 17 9  35
 */

#define SPHINCS_WLEN (SPHINCS_N * 2 + 3)
#define SPHINCS_XH (SPHINCS_H / SPHINCS_D)
#define SPHINCS_TREE_BITS (SPHINCS_H - SPHINCS_XH)
#define SPHINCS_MD_BYTES ((SPHINCS_K * SPHINCS_A + 7) / 8)
#define SPHINCS_TREE_BYTES ((SPHINCS_TREE_BITS + 7) / 8)
#define SPHINCS_LEAF_BYTES ((SPHINCS_XH + 7) / 8)
#define SPHINCS_DIGEST_BYTES (SPHINCS_MD_BYTES + SPHINCS_TREE_BYTES + SPHINCS_LEAF_BYTES)

#endif
