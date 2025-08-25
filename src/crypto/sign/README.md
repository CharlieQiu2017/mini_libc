# Overview of Signature Schemes

When choosing a digital signature scheme for a cryptographic application there are several considerations:

* Security: Is the scheme (believed to be) secure? Is it secure even under quantum attacks?
Is it easy or hard to understand the security proof?
The standard security notion for digital signatures is existential unforgeability under chosen message attacks (EUF-CMA).
However, specific applications may require the scheme to satisfy additional security properties, called "Beyond Unforgeability Features" (BUFF).
See https://eprint.iacr.org/2025/960.pdf for discussions on this point.

* Correctness: Can the signing procedure fail?
Can the signing procedure output a signature that fails to verify?
What is the probability that these events occur?

* Performance: How long does it take to generate a keypair?
How long does it take to sign a message?
How big are the keys and signatures?

* Ease of implementation: A secure implementation of a cryptographic algorithm is usually required to be constant-time.
Depending on the application, it may also need additional protections like masking.
How difficult is it to implement these protections?
In particular, some signature algorithms require rejection-sampling to produce valid signatures.
While rejection-sampling does not constitute a timing vulnerability, it often makes masking more difficult to implement.

* Ease of use: When defining the API of a signature algorithm,
it is usually assumed that the message to be signed is provided in whole.
In practice, when signing a large message it is more convenient to provide the message in shorter chunks.
Thus we need to evaluate how difficult it is to modify the algorithm, so it accepts messages in chunks rather than in whole.

## Security

The two basic questions are "is the scheme secure?" and "is it easy or hard to understand the security proof?"
These are two different questions. A scheme may have large security margins, but its security proof could be very difficult to understand, requiring deep mathematical background.
This creates several problems for security reviewers:
* There are fewer reviewers who are capable of checking the proofs. Then there is a higher chance that an attack escapes the eyes of all reviewers.
* It is more difficult to produce a formally verified proof of its security.
* Not all researchers in the field have the same education background. There is a higher chance that some researcher misunderstands certain details of the proof, and propagates the error into new schemes that are incorrect.

Therefore, in general we prefer schemes that have simpler security proofs, even if they have worse performance.

## Correctness

We prefer schemes that do not have signing failure.
If signing requires multiple attempts to succeed (rejection-sampling), then we prefer schemes where the probability that each attempt succeeds can be formally analyzed.
Ideally, the proof should be simple, relying only on elementary probability theory.
The proof may model hash functions as random oracles.
However, correctness proofs that rely on other computational assumptions should be avoided.

## Performance

Currently, no post-quantum signature scheme achieves the same performance characteristic as the classic EdDSA signature.
An ideal signature scheme should have public keys, private keys, and signatures below 10KB.
The signing time should be on the order of 1M cycles.
The key generation time is less important, but if an application requires ephemeral signature keys, then it should also have fast key generation.

## Ease of Implementation

A good implementation of a cryptographic algorithm should satisfy the following properties:
* No secret-dependent timing variations.
To achieve this feature, the code should not have secret-dependent branching and memory access.
Tools have been developed to check for these properties.
See https://github.com/CharlieQiu2017/doit-enforcer and https://gist.github.com/CharlieQiu2017/0bb50a1966a006affc0746d423e92533.

* If the attacker has local access to the device running the implementation, the implementation should also avoid power side-channels and electromagnetic side-channels.
See https://cryptojedi.org/papers/sca25519-20221104.pdf for the design of an EdDSA implementation that includes strong defenses against these side-channels.

# Comments on Specific Schemes

## Lattice Signatures

By the metrics discussed above, lattice signatures (e.g. Dilithium, Raccoon) are among the *worst* signature schemes.

* Security: While lattice signatures have "provable" security that stem from various worst-case to average-case reductions, there are several issues with these reductions:
  * The reductions require deep mathematical knowledge to understand, and are difficult to formally verify.
  * The reductions are not tight. In fact, the security loss of the reductions is so large that the "reduction proofs" have little relevance to the concrete security of the proposed schemes.
  See Section 9 of https://cr.yp.to/papers/latticeproofs-20190719.pdf.

* Correctness: Almost every lattice-based signature scheme requires rejection-sampling to produce valid signatures.
Moreover, it is difficult to formally analyze the probability that each signing attempt will succeed.
For example, see https://charlieqiu2017.github.io/writing/lyubashevsky-regularity.pdf for an exposition of a difficult lemma that is used in the correctness proof of the Raccoon signature (https://eprint.iacr.org/2024/184.pdf).

  On a side note, the Kyber key-exchange scheme has been formally analyzed in https://eprint.iacr.org/2024/843.pdf.
  However, notice that the authors were unable to prove the decryption failure probability of Kyber claimed in the original paper:
  > We finally note that by “maxing-out” the adversary’s contribution to the noise, we do not need to
  consider the distribution of `cv` at all. However, the distribution of `cu` to the noise expression computed in
  game `CorrectnessBound` still does not match the one in the heuristic computation proposed in [25], where
  `cu` results from compressing a uniform `u`. Justifying this simplification using (Hashed) MLWE is not
  immediate because `r` and `e1` occur elsewhere in the noise expression, so an efficient reduction that does
  not get `r` and `e1` cannot check for the failure event.

  As such even today we do not have a formally verified bound on the decryption failure probability of Kyber.

* Performance: This is the only aspect of lattice signatures that make them stand out among other signature schemes,
and is the primary reason that makes lattice signatures popular.

* Ease of implementation: Since lattice signatures require rejection-sampling,
constant-time implementations must ensure that their secret-dependent branching leaks no information other than the number of attempts used for signing.
This is challenging but still doable.
On the other hand, lattice signatures have features that make masking difficult to implement.
See https://eprint.iacr.org/2024/1291.pdf for discussion of these issues.

  Also, some lattice signatures like FALCON require floating-point computations.
  On most current CPUs, floating-point operations cannot be assume to be constant-time.

* Ease of use: Most lattice signatures follow the hash-and-sign paradigm, so that message chunking is in general not a difficult issue.

## Hash-based Signatures

* Security: Hash-based signatures like SPHINCS+ are believed to have very strong security margins.

* Correctness: Most hash-based signatures are perfectly correct.

* Performance: Hash-based signatures do not have great performance.
The reported signing time of SPHINCS+ is on the order of 200M cycles.
With SIMD optimizations, the signing time is on the order of 50M cycles.
While SPHINCS+ has short secret and public keys, the signature is quite large.
See https://sphincs.org/data/sphincs+-r3.1-specification.pdf.

* Ease of implementation: Most hash-based signatures are easy to implement with constant signing time.

* Ease of use: The signing procedure of SPHINCS+ requires iterating over the input twice.
This may be a problem if reading the input twice is expensive or difficult, for example when the input comes from a streaming source.

## Multivariate Signatures

This category includes the classic Unbalanced Oil and Vinegar (UOV) signature scheme, as well as newer schemes (e.g. MAYO, SNOVA) that aim to optimize the performance of UOV.

* Security: Security of UOV has been analyzed for over 25 years.
It is currently believed to be secure, though it admits no reduction from some hard mathematical problem.
The security of newer multivariate schemes cannot be reduced to the security of UOV.
As such, we are currently uncertain about the security of other multivariate schemes.
Furthermore, multiple multivariate schemes have suffered from fatal attacks in the past few years.
See https://eprint.iacr.org/2022/214 (for Rainbow) and https://eprint.iacr.org/2021/1677 (for GeMSS).

* Correctness: The correctness bound for UOV is relatively easy to analyze.
However, the correctness bounds for many other multivariate schemes such as MAYO and SNOVA are currently unknown.
See https://crypto.stackexchange.com/questions/117301/signing-failure-probability-of-snova.

* Performance: UOV has very fast signing and verification time.
However, it has very large private and public keys.
Therefore, it is almost only suitable for applications with a static set of participants, with public keys fixed and known in advance.

* Ease of implementation: It is mostly easy to produce a constant-time implementation of UOV.
However, the signing procedure requires rejection-sampling.

* Ease of use: Multivariate signatures are mostly unproblematic in this aspect.

## Zero-knowledge and Multiparty Computation Signatures

This category includes schemes where the signature is a zero-knowledge proof of some relation `f(x, a) = y`
where `x` is some secret information, and `a, y` are public information.
The classic EdDSA scheme belongs to this category, but it does not have post-quantum security.
Post-quantum signatures under this category include Picnic, CROSS, LESS, Mirath, MQOM, PERK, RYDE, SDitH, FAEST, etc.

* Security: The security of zero-knowledge proof signatures rely on only two components:
hardness of inverting the underlying relation, and hardness of bypassing the proof protocol.
The proof protocol is usually constructed from a statistically-secure interactive proof protocol (e.g. Sigma protocol) using either Fiat-Shamir transform or Unruh transform.
The classical security of the proof protocols is easy to prove.
For some time, it was unsure whether the Fiat-Shamir transform enjoys post-quantum security.
Therefore, the first versions of the Picnic signature (https://raw.githubusercontent.com/microsoft/Picnic/master/spec/design-v2.2.pdf) used the slower Unruh transform.
However, it was later proved that the Fiat-Shamir transform also enjoys post-quantum security.
See https://eprint.iacr.org/2019/190.pdf.
Thus, to evaluate the security of zero-knowledge proof signatures it is usually sufficient to analyze the hardness of inverting the underlying relation, which is scheme-specific.

* Correctness: Most zero-knowledge proof signatures are perfectly correct.

* Performance: Most zero-knowledge proof signatures do not have great performance.
This is a significant drawback of zero-knowledge proof signatures.

* Ease of implementation: Some zero-knowledge proof schemes have signatures of variable length.
The number of calls they make to the hash function (random oracle) may also be secret-dependent (due to rejection-sampling).
In general, we prefer schemes that have constant signature size and do not use rejection-sampling.

* Ease of use: Most zero-knowledge proof schemes are unproblematic in this aspect.

## Code-based Signatures

This is a subcategory of zero-knowledge proof signatures, where the underlying relation is based on error-correction codes.

* Security: The relations used by various code-based signatures are mostly independent, and their security analyses have not yet stabilized.
* Correctness: Most code-based signatures are perfectly correct.
* Performance: They do not have great performance.
* Ease of implementation: It is usually easy to produce a constant-time implementation.
* Ease of use: Most code-based signatures are unproblematic in this aspect.

## Symmetric-based Signatures

This is a subcategory of zero-knowledge proof signatures, where the underlying relation is based on symmetric ciphers.
The most notable examples are Picnic and FAEST.

* Security: Recovering the key of a symmetric cipher like AES can be considered a very well-understood subject.
Therefore, symmetric-based signatures have very strong security margins.
* Correctness: Most symmetric-based signatures are perfectly correct.
* Performance: They do not have great performance.
* Ease of implementation: It is usually easy to produce a constant-time implementation.
* Ease of use: Most symmetric-based signatures are unproblematic in this aspect.