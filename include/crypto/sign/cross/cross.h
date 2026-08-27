/* The CROSS signature scheme
   https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/round-2/spec-files/cross-spec-round2-web.pdf
   We implement CROSS-R-SDP(G)-1-fast
 */

/* R-SDP(G) is the following problem:
   Given a parity matrix H over F_p, a syndrome s, find a vector e such that s = He.
   Furthermore, e must be a vector of the form \prod_{i = 1}^m a_i^u_i, where "prod" means component-wise product,
   Vectors built this way form an abelian group which we denote by G.
   a_i are fixed vectors, and u_i are non-negative integers.
   All entries of a_i are elements of the form g^k where g is an element of F_p whose multiplicative order is a prime z.
   Hence wlog, we assume 0 <= u_i < z.

   Recall that the ZK protocol for discrete log works as follows.
   To prove that I know a secret r such that a = g^r,
   I first choose a random s and send g^s to the verifier.
   Then, depending on the challenge from the verifier, I reveal either s or s + r.
   The Schnorr protocol generalizes this: the verifier chooses random c and I reveal s + r * c.

   To prove that both He = s and e in G, we use similar techniques to blind e.
   A simple method is as follows. We choose a random vector e' over F_p, and another random vector e'' in G.
   If we commit to both e' and e + e' then we can define e using e = (e + e') - e'.
   In fact, we commit to the value e', e'', e + e', He', H(e + e'), e'' * (e + e'), e'' * e' (Multiplication is component-wise)
   Then the verifier chooses ch:
     If ch == 1, we reveal e' and e''. The verifier checks the commitment of B = He', C = B + s, E = e' * e''. The verifier checks that e'' is in G.
     If ch == 2, we reveal e'' and A = e + e'. The verifier checks the commitment of C = H(e + e'), B = C - s, D = e'' * (e + e'). The verifier checks that e'' is in G.
     If ch == 3, we reveal D = e'' * (e + e') and E = e'' * e'. The verifier computes X = D - E and checks that X in G.

   Extractability:
   Suppose that a potentially-malicious prover can respond to all three challenges correctly.
   Then we have values e', e'', A, B, C, D, E satisfying:
   1. B = He';
   2. C = B + s;
   3. C = HA;
   4. D = e'' * A;
   5. E = e' * e'';
   6. e'' and D - E are in G.

   By 1, 2, 3, we have HA = B + s = He' + s, hence s = H(A - e').
   By 4 we have D = e'' * A, by 5 we have E = e'' * e'. Hence D - E = e'' * (A - e'). By 6, e'' * (A - e') and e'' are in G. Hence A - e' is in G.
   This gives us s = H(A - e') and A - e' is in G.

   Zero-knowledge:
   For ch == 1: both e' and e'' are uniformly random.
   For ch == 2: both e + e' and e'' are uniformly random.
   For ch == 3: Notice that e'' * e is uniformly random over G. Each value of e'' * e implicitly defines a value of e''.
		Given any value of e'', e'' * (e + e') is uniformly random since e' is uniformly random.
		Then e'' * e' can be computed as e'' * [(e + e') - e] = e'' * (e + e') - e'' * e.

   If the prover is dishonest, it has a success probability of at most 2/3.
 */

/* The CROSS-ID protocol uses a more elaborate method.
   The prover chooses a random e' in G.
   Define v = e * e'^-1, so that v represents e masked by e'^-1.
   If we reveal v and e' individually, then the verifier may check that both values are in G.
   Hence e is defined by v * e'.

   If we reveal both v and e', then the verifier may compute e = v * e', and confirm that He = s.
   However this would reveal the secret e which is undesirable.
   Thus we mask e' using another random vector u'.
   Define y = u' + e', then He = H(v * e') = H(v * (y - u')) = H(v * y) - H(v * u').

   Hence, we commit to v, e', u', y, H(v * u').
   The verifier needs to check the following relations:
   1. v and e' are in G.
   2. y = e' + u'.
   3. H(v * y) = H(v * u') + s.
   This can be done via 3 challenges:
   Challenge 1: Reveal v, y, H(v * u'), check that v is in G and H(v * y) = H(v * u') + s.
   Challenge 2: Reveal v, u', H(v * u'), check that v is in G and H(v * u') is correct.
   Challenge 3: Reveal e', u', y, check that e' is in G and y = e' + u'.

   Extractability:
   Suppose that a potentially-malicious prover can respond to all three challenges correctly.
   Then we have values v, e', u', X, Y satisfying:
   1. v and e' are in G.
   2. X = e' + u'.
   3. Y = H(v * u').
   4. H(v * X) = Y + s.
   By 1 we have (v * e') in G. By 2,3,4 we have s = H(v * (X - u')) = H(v * e').
   This gives us s = H(v * e') and v * e' in G.

   Zero-knowledge:
   For challenge 1: both v and y are uniformly random. H(v * u') = H(v * y) - s.
   For challenge 2: both v and u' are uniformly random.
   For challenge 3: both e' and u' are uniformly random.

   A further twist of CROSS is to introduce a random coefficient ch in front of s.
   Hence: we define y = ch * e' + u', so that H(v * y) = H(v * u') + ch * s.
   The challenges 1 and 2 above can be seen as special cases ch == 1 and ch == 0.
   Since the values v, e', u', H(v * u') are unrelated to ch, we let the prover commit to these values first.
   Then the verifier chooses a random ch, and the prover commits to the value y.
   Now the verifier chooses a random bit ch', and depending on the bit let the prover reveal either v, y, H(v * u'), or e', u', y.
   Suppose that a potentially malicious prover can respond to both ch' challenges under two different ch values correctly.
   Then we have values v, e', u', X1, X2, Y satisfying:
   1. v and e' are in G.
   2. X1 = ch1 * e' + u'.
   3. X2 = ch2 * e' + u'.
   4. H(v * X1) = Y + ch1 * s.
   5. H(v * X2) = Y + ch2 * s.

   By 1 we have (v * e') in G. By 2,4 we have ch1 * s = H(v * X1) - Y = H(v * (ch1 * e' + u')) - Y = ch1 * H(v * e') + H(v * u') - Y
   Similarly, ch2 * s = ch2 * H(v * e') + H(v * u') - Y.
   Subtracting the two equations we get (ch1 - ch2) * s = (ch1 - ch2) * H(v * e'). Hence s = H(v * e').

   The CROSS specification requires ch to be non-zero. However, nothing in the security proof requires ch to be non-zero.
   We have contacted the authors of CROSS and they have confirmed that allowing ch == 0 is secure.
   Therefore we implement the variant that allows ch == 0.
   This is deliberately incompatible with reference implementation.
 */

/* The total number of ZK rounds is given by CROSS_T.
   Recall that in each round, the prover first needs to commit to v, e', u', H(v * u').
   The commitment to v and H(v * u') is called cmt0.
   To commit to e' and u' we simply commit to the seed that was used to generate them, and this is called cmt1.
   Thus a simple implementation of CROSS could be as follows:
   Let digest_1 = Hash(msg || salt || cmt0[0] || ... || cmt0[CROSS_T - 1] || cmt1[0] || ... || cmt1[CROSS_T - 1]),
   And we generate the first challenge ch with CSPRNG(digest_1).
   Then let digest_2 = Hash(y[0] || ... || y[CROSS_T - 1] || digest_1),
   and we generate the second challenge ch' with CSPRNG(digest_2).
   For each round such that ch' == 0 we have to reveal v, H(v * u'), y,
   and for each round such that ch' == 1 we have to reveal seed, y.
   We also have to provide the commitments cmt0 or cmt1 of unrevealed values.
   The signature thus consists of (salt, digest_1, digest_2, unrevealed cmt0, unrevealed cmt1, responses).

   CROSS provides three performance profiles called "fast", "balanced", "small".
   The "fast" version is very similar to what is described above.
   However, the "balanced" and "small" versions introduce more techniques to reduce the signature size.
   We only implement the "fast" variant at the moment, but we document how "balanced" and "small" works here.

   First the prover chooses a seed. Then it iteratively expands the seed into a tree structure,
   such that the leaves of the tree are used as the seed of each round.
   This is the "SeedLeaves" function in the spec, and the tree is called the "seed tree".

           Master seed
             /    \
            /      \
           /        \
  Middle node       Middle node
       /   \           /   \
      /     \         /     \
     /       \       /       \
   seed[0] seed[1] seed[2] seed[3]

   The prover then builds a Merkle-tree of the cmt0 commitments for the per-round seeds.

           cmt0 tree root
             /    \
            /      \
           /        \
  Middle node       Middle node
       /   \           /   \
      /     \         /     \
     /       \       /       \
   cmt0[0] cmt0[1] cmt0[2] cmt0[3]

   We shall call this tree the "commit tree", and its tree root is computed by the "TreeRoot" function in the spec.

   Let digest_cmt0 = cmt0 tree root, and digest_cmt1 = Hash(cmt1[0] || ... || cmt1[CROSS_T - 1]),
   and digest_1 is actually computed by Hash(Hash(msg) || Hash(digest_cmt0 || digest_cmt1) || salt).
   The computation of digest_2 is still Hash(y[0] || ... || y[CROSS_T - 1] || digest_1).
   Now to prove digest_cmt0 is computed correctly, we do not need to reveal all cmt0 values.
   The verifier randomly chooses a few branches to open, and the prover opens these branches
   along with sibling middle nodes to allow recomputing the tree root, and this is called a "Merkle proof".
   Now CROSS uses a "fixed-weight" challenge structure that is heavily skewed toward ch' == 1.
   This means we need to open a large number of seed values and only a small number of (v, H(v * u')) values.

   To open a large number of seed values, we represent them as a sub-forest of the seed tree, and we open the roots of this sub-forest.
   This is the "SeedPath" function in the spec, and its output is the "Path" component of the signature.
   The verifier recomputes the leaves with the "RebuildLeaves" function in the spec.

   The small number of (v, H(v * u')) values we reveal forms the Merkle branch openings,
   and we only need to provide a few additional middle nodes to allow recomputing the root.
   This is computed by the "TreeProof" function in the spec, and its output is the "Proof" component of the signature.
   The verifier reconstructs the commit tree root using the "RecomputeRoot" function in the spec.

   We still have to providde all cmt1 commitments whose corresponding seed is not revealed.
   They are provided in the response to each round with ch' == 0.
 */

#ifndef CROSS_CROSS_H
#define CROSS_CROSS_H

#include <stdint.h>
#include <stddef.h>
#include <crypto/sign/cross/parameters.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The private key is a single seed of length KEYPAIR_SEED_SIZE == LAMBDA / 4 == 32 bytes.
   This single seed is used to generate both seed_pk and the secret vector e.
 */

struct cross_sk_t {
  uint8_t seed_sk[CROSS_KEYPAIR_SEED_SIZE];
};

/* The public key consists of seed_pk (used to generate the public matrix H and vectors a_i),
   and the syndrome vector s (stored in packed format).
 */

struct cross_pk_t {
  uint8_t seed_pk[CROSS_KEYPAIR_SEED_SIZE]; /* 32 bytes */
  uint8_t s[CROSS_PACKED_SYN_SIZE]; /* 22 bytes */
};

/* The signature is (salt, digest_cmt, digest_chall2, Path, Proof, Resp).
   digest_cmt == Hash(digest_cmt0 || digest_cmt1),
   Path is a list of seed values which are revealed to the verifier,
   Proof is a list of cmt0 values ("fast" variant does not use Merkle proof),
   Resp is the response to each round with ch' == 0,
   which consists of two parts resp_0 (v and y values) and resp_1 (the cmt1 commitment).
   The response to each round with ch' == 1 is wholly contained in Path and Proof.
   CROSS uses "fixed-weight" challenge and the number of rounds with ch' == 1 is exactly CROSS_W.
   The spec document does not fully specify the signature format,
   and the following struct definitions are taken from the reference implementation.
 */

struct cross_resp_0_t {
  uint8_t y[CROSS_PACKED_RESP_Y_SIZE]; /* 62 bytes */
  uint8_t v[CROSS_PACKED_RESP_V_SIZE]; /* 22 bytes */
};

struct cross_sig_t {
  uint8_t salt[CROSS_SALT_SIZE]; /* 32 bytes */
  uint8_t digest_cmt[CROSS_HASH_DIGEST_SIZE]; /* 32 bytes */
  uint8_t digest_chall2[CROSS_HASH_DIGEST_SIZE]; /* 32 bytes */
  uint8_t path[CROSS_W * CROSS_SEED_SIZE]; /* 76 * 16 = 1216 bytes */
  uint8_t proof[CROSS_W * CROSS_HASH_DIGEST_SIZE]; /* 76 * 32 = 2432 bytes */
  uint8_t resp_1[CROSS_T - CROSS_W][CROSS_HASH_DIGEST_SIZE]; /* 71 * 32 = 2272 bytes */
  struct cross_resp_0_t resp_0[CROSS_T - CROSS_W]; /* 71 * 84 = 5964 bytes */
};

/* Total signature size: 11980 bytes */

void cross_rsdpg_1_fast_gen_key_from_seed (const struct cross_sk_t * sk, struct cross_pk_t * pk_out);
void cross_rsdpg_1_fast_gen_key (struct cross_sk_t * sk_out, struct cross_pk_t * pk_out);
void cross_rsdpg_1_fast_sign_with_seed_salt (const struct cross_sk_t * sk, const uint8_t * seed, const uint8_t * salt, const unsigned char * msg, size_t msg_len, struct cross_sig_t * sig_out);
void cross_rsdpg_1_fast_sign (const struct cross_sk_t * sk, const unsigned char * msg, size_t msg_len, struct cross_sig_t * sig_out);
_Bool cross_rsdpg_1_fast_verify (const struct cross_pk_t * pk, const unsigned char * msg, size_t msg_len, const struct cross_sig_t * sig);

#ifdef __cplusplus
}
#endif

#endif
