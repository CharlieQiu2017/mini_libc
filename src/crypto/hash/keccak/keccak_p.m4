`
/* Keccak-p permutations.
   Adapted from mupq/common/keccakf1600.c
 */
'

`
#include <stdint.h>
#include <crypto/hash/keccak/keccak_p.h>
'

`
#define ROL(T, a, offset) (((a) << ((offset) % (8 * sizeof (T)))) ^ ((a) >> (8 * sizeof (T) - ((offset) % (8 * sizeof (T))))))
'

`
static const uint64_t keccak_p_rc[24] = {
  (uint64_t) 0x0000000000000001ull,
  (uint64_t) 0x0000000000008082ull,
  (uint64_t) 0x800000000000808aull,
  (uint64_t) 0x8000000080008000ull,
  (uint64_t) 0x000000000000808bull,
  (uint64_t) 0x0000000080000001ull,
  (uint64_t) 0x8000000080008081ull,
  (uint64_t) 0x8000000000008009ull,
  (uint64_t) 0x000000000000008aull,
  (uint64_t) 0x0000000000000088ull,
  (uint64_t) 0x0000000080008009ull,
  (uint64_t) 0x000000008000000aull,
  (uint64_t) 0x000000008000808bull,
  (uint64_t) 0x800000000000008bull,
  (uint64_t) 0x8000000000008089ull,
  (uint64_t) 0x8000000000008003ull,
  (uint64_t) 0x8000000000008002ull,
  (uint64_t) 0x8000000000000080ull,
  (uint64_t) 0x000000000000800aull,
  (uint64_t) 0x800000008000000aull,
  (uint64_t) 0x8000000080008081ull,
  (uint64_t) 0x8000000000008080ull,
  (uint64_t) 0x0000000080000001ull,
  (uint64_t) 0x8000000080008008ull
};
'

`
/* Code for different versions of Keccak-p[b, n_r] are almost the same except the types used for the lanes and the round number.
   Therefore, we define a macro that expands to the correct function body.
   State of Keccak is a bit-array A[x, y, z], with 0 <= x < 5, 0 <= y < 5, 0 <= z < lane_size (8, 16, 32, 64 depending on permutation).
   In this implementation, A[x, y, z] = state[x + 5 * y] & (1ull << z).
 */
'

define(`BODY',`
  T Aba, Abe, Abi, Abo, Abu;
  T Aga, Age, Agi, Ago, Agu;
  T Aka, Ake, Aki, Ako, Aku;
  T Ama, Ame, Ami, Amo, Amu;
  T Asa, Ase, Asi, Aso, Asu;
  T BCa, BCe, BCi, BCo, BCu;
  T Da, De, Di, Do, Du;
  T Eba, Ebe, Ebi, Ebo, Ebu;
  T Ega, Ege, Egi, Ego, Egu;
  T Eka, Eke, Eki, Eko, Eku;
  T Ema, Eme, Emi, Emo, Emu;
  T Esa, Ese, Esi, Eso, Esu;

  /* copyFromState(A, state) */
  Aba = state[ 0];
  Abe = state[ 1];
  Abi = state[ 2];
  Abo = state[ 3];
  Abu = state[ 4];
  Aga = state[ 5];
  Age = state[ 6];
  Agi = state[ 7];
  Ago = state[ 8];
  Agu = state[ 9];
  Aka = state[10];
  Ake = state[11];
  Aki = state[12];
  Ako = state[13];
  Aku = state[14];
  Ama = state[15];
  Ame = state[16];
  Ami = state[17];
  Amo = state[18];
  Amu = state[19];
  Asa = state[20];
  Ase = state[21];
  Asi = state[22];
  Aso = state[23];
  Asu = state[24];

  for (uint32_t round = STROUND; round < STROUND + NROUND; round += 2) {
    /* prepareTheta */
    BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    /* thetaRhoPiChiIotaPrepareTheta (round, A, E) */
    Da = BCu ^ ROL (T, BCe, 1);
    De = BCa ^ ROL (T, BCi, 1);
    Di = BCe ^ ROL (T, BCo, 1);
    Do = BCi ^ ROL (T, BCu, 1);
    Du = BCo ^ ROL (T, BCa, 1);

    Aba ^= Da;
    BCa = Aba;
    Age ^= De;
    BCe = ROL (T, Age, 44);
    Aki ^= Di;
    BCi = ROL (T, Aki, 43);
    Amo ^= Do;
    BCo = ROL (T, Amo, 21);
    Asu ^= Du;
    BCu = ROL (T, Asu, 14);
    Eba = BCa ^ ((~ BCe) & BCi);
    Eba ^= keccak_p_rc[round];
    Ebe = BCe ^ ((~ BCi) & BCo);
    Ebi = BCi ^ ((~ BCo) & BCu);
    Ebo = BCo ^ ((~ BCu) & BCa);
    Ebu = BCu ^ ((~ BCa) & BCe);

    Abo ^= Do;
    BCa = ROL (T, Abo, 28);
    Agu ^= Du;
    BCe = ROL (T, Agu, 20);
    Aka ^= Da;
    BCi = ROL (T, Aka, 3);
    Ame ^= De;
    BCo = ROL (T, Ame, 45);
    Asi ^= Di;
    BCu = ROL (T, Asi, 61);
    Ega = BCa ^ ((~ BCe) & BCi);
    Ege = BCe ^ ((~ BCi) & BCo);
    Egi = BCi ^ ((~ BCo) & BCu);
    Ego = BCo ^ ((~ BCu) & BCa);
    Egu = BCu ^ ((~ BCa) & BCe);

    Abe ^= De;
    BCa = ROL (T, Abe, 1);
    Agi ^= Di;
    BCe = ROL (T, Agi, 6);
    Ako ^= Do;
    BCi = ROL (T, Ako, 25);
    Amu ^= Du;
    BCo = ROL (T, Amu, 8);
    Asa ^= Da;
    BCu = ROL (T, Asa, 18);
    Eka = BCa ^ ((~ BCe) & BCi);
    Eke = BCe ^ ((~ BCi) & BCo);
    Eki = BCi ^ ((~ BCo) & BCu);
    Eko = BCo ^ ((~ BCu) & BCa);
    Eku = BCu ^ ((~ BCa) & BCe);

    Abu ^= Du;
    BCa = ROL (T, Abu, 27);
    Aga ^= Da;
    BCe = ROL (T, Aga, 36);
    Ake ^= De;
    BCi = ROL (T, Ake, 10);
    Ami ^= Di;
    BCo = ROL (T, Ami, 15);
    Aso ^= Do;
    BCu = ROL (T, Aso, 56);
    Ema = BCa ^ ((~ BCe) & BCi);
    Eme = BCe ^ ((~ BCi) & BCo);
    Emi = BCi ^ ((~ BCo) & BCu);
    Emo = BCo ^ ((~ BCu) & BCa);
    Emu = BCu ^ ((~ BCa) & BCe);

    Abi ^= Di;
    BCa = ROL (T, Abi, 62);
    Ago ^= Do;
    BCe = ROL (T, Ago, 55);
    Aku ^= Du;
    BCi = ROL (T, Aku, 39);
    Ama ^= Da;
    BCo = ROL (T, Ama, 41);
    Ase ^= De;
    BCu = ROL (T, Ase, 2);
    Esa = BCa ^ ((~ BCe) & BCi);
    Ese = BCe ^ ((~ BCi) & BCo);
    Esi = BCi ^ ((~ BCo) & BCu);
    Eso = BCo ^ ((~ BCu) & BCa);
    Esu = BCu ^ ((~ BCa) & BCe);

    /* prepareTheta */
    BCa = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    BCe = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    BCi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    BCo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    BCu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    /* thetaRhoPiChiIotaPrepareTheta(round + 1, E, A) */
    Da = BCu ^ ROL (T, BCe, 1);
    De = BCa ^ ROL (T, BCi, 1);
    Di = BCe ^ ROL (T, BCo, 1);
    Do = BCi ^ ROL (T, BCu, 1);
    Du = BCo ^ ROL (T, BCa, 1);

    Eba ^= Da;
    BCa = Eba;
    Ege ^= De;
    BCe = ROL (T, Ege, 44);
    Eki ^= Di;
    BCi = ROL (T, Eki, 43);
    Emo ^= Do;
    BCo = ROL (T, Emo, 21);
    Esu ^= Du;
    BCu = ROL (T, Esu, 14);
    Aba = BCa ^ ((~ BCe) & BCi);
    Aba ^= keccak_p_rc[round + 1];
    Abe = BCe ^ ((~ BCi) & BCo);
    Abi = BCi ^ ((~ BCo) & BCu);
    Abo = BCo ^ ((~ BCu) & BCa);
    Abu = BCu ^ ((~ BCa) & BCe);

    Ebo ^= Do;
    BCa = ROL (T, Ebo, 28);
    Egu ^= Du;
    BCe = ROL (T, Egu, 20);
    Eka ^= Da;
    BCi = ROL (T, Eka, 3);
    Eme ^= De;
    BCo = ROL (T, Eme, 45);
    Esi ^= Di;
    BCu = ROL (T, Esi, 61);
    Aga = BCa ^ ((~ BCe) & BCi);
    Age = BCe ^ ((~ BCi) & BCo);
    Agi = BCi ^ ((~ BCo) & BCu);
    Ago = BCo ^ ((~ BCu) & BCa);
    Agu = BCu ^ ((~ BCa) & BCe);

    Ebe ^= De;
    BCa = ROL (T, Ebe, 1);
    Egi ^= Di;
    BCe = ROL (T, Egi, 6);
    Eko ^= Do;
    BCi = ROL (T, Eko, 25);
    Emu ^= Du;
    BCo = ROL (T, Emu, 8);
    Esa ^= Da;
    BCu = ROL (T, Esa, 18);
    Aka = BCa ^ ((~ BCe) & BCi);
    Ake = BCe ^ ((~ BCi) & BCo);
    Aki = BCi ^ ((~ BCo) & BCu);
    Ako = BCo ^ ((~ BCu) & BCa);
    Aku = BCu ^ ((~ BCa) & BCe);

    Ebu ^= Du;
    BCa = ROL (T, Ebu, 27);
    Ega ^= Da;
    BCe = ROL (T, Ega, 36);
    Eke ^= De;
    BCi = ROL (T, Eke, 10);
    Emi ^= Di;
    BCo = ROL (T, Emi, 15);
    Eso ^= Do;
    BCu = ROL (T, Eso, 56);
    Ama = BCa ^ ((~ BCe) & BCi);
    Ame = BCe ^ ((~ BCi) & BCo);
    Ami = BCi ^ ((~ BCo) & BCu);
    Amo = BCo ^ ((~ BCu) & BCa);
    Amu = BCu ^ ((~ BCa) & BCe);

    Ebi ^= Di;
    BCa = ROL (T, Ebi, 62);
    Ego ^= Do;
    BCe = ROL (T, Ego, 55);
    Eku ^= Du;
    BCi = ROL (T, Eku, 39);
    Ema ^= Da;
    BCo = ROL (T, Ema, 41);
    Ese ^= De;
    BCu = ROL (T, Ese, 2);
    Asa = BCa ^ ((~ BCe) & BCi);
    Ase = BCe ^ ((~ BCi) & BCo);
    Asi = BCi ^ ((~ BCo) & BCu);
    Aso = BCo ^ ((~ BCu) & BCa);
    Asu = BCu ^ ((~ BCa) & BCe);
  }

  /* If NROUND is odd, do one more round */
  if (NROUND % 2) {
    /* prepareTheta */
    BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    /* thetaRhoPiChiIotaPrepareTheta (round, A, E) */
    Da = BCu ^ ROL (T, BCe, 1);
    De = BCa ^ ROL (T, BCi, 1);
    Di = BCe ^ ROL (T, BCo, 1);
    Do = BCi ^ ROL (T, BCu, 1);
    Du = BCo ^ ROL (T, BCa, 1);

    Aba ^= Da;
    BCa = Aba;
    Age ^= De;
    BCe = ROL (T, Age, 44);
    Aki ^= Di;
    BCi = ROL (T, Aki, 43);
    Amo ^= Do;
    BCo = ROL (T, Amo, 21);
    Asu ^= Du;
    BCu = ROL (T, Asu, 14);
    Eba = BCa ^ ((~ BCe) & BCi);
    Eba ^= keccak_p_rc[STROUND + NROUND - 1];
    Ebe = BCe ^ ((~ BCi) & BCo);
    Ebi = BCi ^ ((~ BCo) & BCu);
    Ebo = BCo ^ ((~ BCu) & BCa);
    Ebu = BCu ^ ((~ BCa) & BCe);

    Abo ^= Do;
    BCa = ROL (T, Abo, 28);
    Agu ^= Du;
    BCe = ROL (T, Agu, 20);
    Aka ^= Da;
    BCi = ROL (T, Aka, 3);
    Ame ^= De;
    BCo = ROL (T, Ame, 45);
    Asi ^= Di;
    BCu = ROL (T, Asi, 61);
    Ega = BCa ^ ((~ BCe) & BCi);
    Ege = BCe ^ ((~ BCi) & BCo);
    Egi = BCi ^ ((~ BCo) & BCu);
    Ego = BCo ^ ((~ BCu) & BCa);
    Egu = BCu ^ ((~ BCa) & BCe);

    Abe ^= De;
    BCa = ROL (T, Abe, 1);
    Agi ^= Di;
    BCe = ROL (T, Agi, 6);
    Ako ^= Do;
    BCi = ROL (T, Ako, 25);
    Amu ^= Du;
    BCo = ROL (T, Amu, 8);
    Asa ^= Da;
    BCu = ROL (T, Asa, 18);
    Eka = BCa ^ ((~ BCe) & BCi);
    Eke = BCe ^ ((~ BCi) & BCo);
    Eki = BCi ^ ((~ BCo) & BCu);
    Eko = BCo ^ ((~ BCu) & BCa);
    Eku = BCu ^ ((~ BCa) & BCe);

    Abu ^= Du;
    BCa = ROL (T, Abu, 27);
    Aga ^= Da;
    BCe = ROL (T, Aga, 36);
    Ake ^= De;
    BCi = ROL (T, Ake, 10);
    Ami ^= Di;
    BCo = ROL (T, Ami, 15);
    Aso ^= Do;
    BCu = ROL (T, Aso, 56);
    Ema = BCa ^ ((~ BCe) & BCi);
    Eme = BCe ^ ((~ BCi) & BCo);
    Emi = BCi ^ ((~ BCo) & BCu);
    Emo = BCo ^ ((~ BCu) & BCa);
    Emu = BCu ^ ((~ BCa) & BCe);

    Abi ^= Di;
    BCa = ROL (T, Abi, 62);
    Ago ^= Do;
    BCe = ROL (T, Ago, 55);
    Aku ^= Du;
    BCi = ROL (T, Aku, 39);
    Ama ^= Da;
    BCo = ROL (T, Ama, 41);
    Ase ^= De;
    BCu = ROL (T, Ase, 2);
    Esa = BCa ^ ((~ BCe) & BCi);
    Ese = BCe ^ ((~ BCi) & BCo);
    Esi = BCi ^ ((~ BCo) & BCu);
    Eso = BCo ^ ((~ BCu) & BCa);
    Esu = BCu ^ ((~ BCa) & BCe);

    /* copyToState(state, E) */
    state[ 0] = Eba;
    state[ 1] = Ebe;
    state[ 2] = Ebi;
    state[ 3] = Ebo;
    state[ 4] = Ebu;
    state[ 5] = Ega;
    state[ 6] = Ege;
    state[ 7] = Egi;
    state[ 8] = Ego;
    state[ 9] = Egu;
    state[10] = Eka;
    state[11] = Eke;
    state[12] = Eki;
    state[13] = Eko;
    state[14] = Eku;
    state[15] = Ema;
    state[16] = Eme;
    state[17] = Emi;
    state[18] = Emo;
    state[19] = Emu;
    state[20] = Esa;
    state[21] = Ese;
    state[22] = Esi;
    state[23] = Eso;
    state[24] = Esu;
  } else {
    /* copyToState(state, A) */
    state[ 0] = Aba;
    state[ 1] = Abe;
    state[ 2] = Abi;
    state[ 3] = Abo;
    state[ 4] = Abu;
    state[ 5] = Aga;
    state[ 6] = Age;
    state[ 7] = Agi;
    state[ 8] = Ago;
    state[ 9] = Agu;
    state[10] = Aka;
    state[11] = Ake;
    state[12] = Aki;
    state[13] = Ako;
    state[14] = Aku;
    state[15] = Ama;
    state[16] = Ame;
    state[17] = Ami;
    state[18] = Amo;
    state[19] = Amu;
    state[20] = Asa;
    state[21] = Ase;
    state[22] = Asi;
    state[23] = Aso;
    state[24] = Asu;
  }
')dnl

define(`INST',`
void keccak_p_$1_permute ($2 * state) {
#define T $2
#define NROUND $3
#define STROUND 0
BODY
#undef T
#undef NROUND
#undef STROUND
}')dnl

INST(200, uint8_t, 18)
INST(400, uint16_t, 20)
INST(800, uint32_t, 22)
INST(1600, uint64_t, 24)

define(`INST_EXT',`
void keccak_p_$1_$3_permute ($2 * state) {
#define T $2
#define NROUND $3
#define STROUND $4
BODY
#undef T
#undef NROUND
#undef STROUND
}')dnl

INST_EXT(800, uint32_t, 12, 10)
INST_EXT(1600, uint64_t, 12, 12)
INST_EXT(1600, uint64_t, 6, 18)
