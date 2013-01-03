/*
  Copyright 1995-2012 Mithral Communications & Design Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef COSM_SECURITY_H
#define COSM_SECURITY_H

#include "cosm/cputypes.h"
#include "cosm/os_task.h"
#include "cosm/buffer.h"
#include "cosm/transform.h"
#include "cosm/bignum.h"

/* Hash Functions */

PACKED_STRUCT_BEGIN
typedef struct cosm_HASH
{
  u8 hash[32]; /* max 256 bits, big endian, aligned left */
} cosm_HASH;
PACKED_STRUCT_END

s32 CosmHashEq( const cosm_HASH * hashA, const cosm_HASH * hashB );
  /*
    Check if the hashes are equal. hashA == hashB ? 1 : 0;
    Returns: 1 if the hashes are equal, or 0 if they are not equal.
  */

/* Random functions */

typedef struct cosm_PRNG
{
  u8 pool[16]; /* as much as MD5 does per pass */
} cosm_PRNG;

s32 CosmPRNG( cosm_PRNG * prng, void * data, u64 length,
  const void * salt, u64 salt_length );
  /*
    Generate length bytes of deterministic psuedo-random data.
    Initialization of the generator is done by
    feeding it salt_length bytes of salt. Any salt fed to the
    generator will be used before the data is generated, and is also
    cumulative, so it can be called repeatedly with additional salt
    to provide more initial randomness. Given the same salt, the same
    bytes will always come out.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmEntropy( u8 * data, u64 length );
  /*
    Generate length bytes of random data on systems capable of doing this.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* Encryption functions are transforms, but these will help */
/* All operate on 128 bit blocks */

#define COSM_CRYPTO_MODE_CFB  19 /* 8-bit cipher-feedback, for streams */
#define COSM_CRYPTO_MODE_CBC  59 /* Cipher block chainging, for files */
#define COSM_CRYPTO_MODE_ECB  79 /* Electronic codebook, for keys & random */

#define COSM_CRYPTO_ENCRYPT   179
#define COSM_CRYPTO_DECRYPT   199

/* PKI functions, keys, signatures, keyrings */

/* A key is identified by the bits, id, and create time */

typedef struct cosm_RSA_KEY
{
  u16 pkt_type;   /* DO NOT CHANGE ANY OF THIS! */
  u16 pkt_version;
  u32 bits;
  s64 create;
  u64 id;
  s64 expire;
  utf8 alias[32]; /* 64 byte header, size = bits/16 */
  cosm_BN n;      /* bits, public, [64] */
  cosm_BN d;      /* bits, e^(-1) mod (p-1)(q-1), [size*2+64]*/
  cosm_BN p;      /* bits/2, p > q */
  cosm_BN q;      /* bits/2, q < p */
  cosm_BN dmp1;   /* bits/2, d mod (p-1) */
  cosm_BN dmq1;   /* bits/2, d mod (q-1) */
  cosm_BN iqmp;   /* bits/2, inverse of q mod p */
  u8 iv[16];      /* 128 bits of IV for AES, [size*9+64] */
  u8 checksum[4]; /* CRC32 to see if we properly decrypted, [size*9+80] */
} cosm_RSA_KEY; /* total = size*9+84 on disk */

typedef struct cosm_RSA_SIG
{
  u16 pkt_type;
  u16 pkt_version;
  u32 bits;
  s64 create;
  u64 id;
  s64 timestamp;
  u8 type;
  u8 shared;
  cosm_BN sig;
} cosm_RSA_SIG; /* 34 + bits/8 bytes on disk */

#define COSM_RSA_VERSION  1

#define COSM_RSA_ERROR_PARAM       -1 /* A paramerter was invalid */
#define COSM_RSA_ERROR_MEMORY      -2 /* Memory problem */
#define COSM_RSA_ERROR_FORMAT      -3 /* Key/Sig isn't right, size wrong */
#define COSM_RSA_ERROR_EXPIRED     -4 /* Key expired */
#define COSM_RSA_ERROR_PASSPHRASE  -5 /* Wrong passphrase */

#define COSM_RSA_PUBLIC       0x0005 /* Public key */
#define COSM_RSA_PRIVATE      0x000A /* Private key (pub + private) */
#define COSM_RSA_SIGNATURE    0x0050 /* Signature */

#define COSM_RSA_SHARED_NO      0x33 /* Signatere is valid to signer only */
#define COSM_RSA_SHARED_YES     0xCC /* Signatere is valid for any reader */

#define COSM_RSA_SIG_MESSAGE    0x00 /* Sending data to the key owner */
#define COSM_RSA_SIG_SIGN       0x05 /* Signer signs data */
#define COSM_RSA_SIG_KNOWN      0x06 /* Signer untrusted but known data */
#define COSM_RSA_SIG_WEAK       0x09 /* Signer weakly trusts the data */
#define COSM_RSA_SIG_STRONG     0x0A /* Signer strongly trusts the data */
#define COSM_RSA_SIG_TIMESTAMP  0x0F /* Signer says data existed only */
#define COSM_RSA_SIG_REVOKE     0xFF /* Signer revokes matching signature */

/* keys */

s32 CosmRSAKeyGen( cosm_RSA_KEY * public_key, cosm_RSA_KEY * private_key,
  u32 bits, const u8 * rnd_bits, u64 id, const utf8 * alias,
  cosmtime create, cosmtime expire, void (*callback)( s32, s32, void * ),
  void * callback_param );
  /*
    Generate public_key and private_key with a bits bit n.
    Please read this entire description, as many parameters have
    special meanings and restrictions. Knowing what you're doing before
    using any PKI functions would be a very good idea.

    bits must be at least 512, and a power of 2.
    Keys less then 1024bits are now considered breakable by large
    organizations, and 512 bit keys are breakable with a few computers
    in a few weeks, so 2048 and 4096 bit keys are recommended.

    rnd_bits should be bits bits of random data.
    It is VERY important that the user take great care to make sure
    that these bits are truely random, and not based on any data
    such as the time. CosmPRNG and CosmEntropy are also not enough on
    their own.

    id has specific ranges and meanings:
    0    to 2^31-1: local human ID.
    2^31 to 2^32-1: local machine ID.
    2^32 to 2^63-1: global Cosm-assigned human ID.
    2^63 to 2^64-1: global Cosm-assigned machine ID.
    When printed in hex, local IDs are 8 digits, and global ID are 16 digits.
    Local ID are use within each organization, often something like an
    employee number, or a customer number for example. Global IDs should
    not be used to identify people once they are in a local domain, the local
    ID should be used instead for privacy and security reasons.
    That something is using a human ID does NOT imply they are an adult
    that can sign contracts, nor any form of intelligence.

    create is the time of key creation and should always
    be the current time. expire is the expiration time
    of the keys. CosmRSAEncode() will not be allowed with an
    expired key. Most keys should have a lifetime of 1 year.

    A key is uniquely identified by the bits,
    id, and create. This means that keys
    should not be generated more then once a second, this should not
    be a big concern. You should also never generate a key with a
    id you haven't been assigned, because it will not
    result in a valid key.

    alias is a max 15 byte utf8 string that has no
    special meaning except to the key's owner as a tool to quickly
    tell keys apart. No special meaning should be given beyond that.

    callback and callback_param provide information
    on the possibly long key generation process. They work as decribed
    in CosmBNPrimeGen, with the added calls of
    callback( 3, 0, callback_param ) when the keypair test begins and
    callback( 4, 0, callback_param ) when the test finishes.
    Note that 2 primes will be generated during key creation.

    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmRSAKeyLoad( cosm_RSA_KEY * key, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes, const cosm_HASH * pass_hash );
  /*
    Load the key from the buffer into key, reading at most
    max_bytes bytes. bytes_read will be set to the number of bytes
    that were in the key.
    Loading private keys also requires the COSM_HASH_SHA256 hash of
    the users passphrase pass_hash in order to decrypt the
    private data. When loading a public key passhash is ignored.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmRSAKeySave( u8 * buffer, u64 * length, const cosm_RSA_KEY * key,
  const u64 max_bytes, const u8 * rnd_bytes,
  const cosm_HASH * pass_hash );
  /*
    Save the key key into the buffer, writing at most max_bytes bytes.
    length will be set to the number of bytes written.
    Private keys also requite 16 bytes of random data rnd_bytes,
    and a hash pass_hash which is the COSM_HASH_SHA256 hash of
    the users passphrase in order to encrypt the private data.
    When saving a public key these two parameters are ignored.
    Returns: COSM_PASS on success, or an error code on failure.
  */

void CosmRSAKeyFree( cosm_RSA_KEY * key );
  /*
    Free all internal key data, and zero the key structure.
    Returns: nothing.
  */

/* signatures */

s32 CosmRSASigLoad( cosm_RSA_SIG * sig, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes );
  /*
    Load the signature from the buffer into sig, reading at most
    max_bytes bytes. bytes_read will be set to the number of bytes
    that were in the signature.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmRSASigSave( u8 * buffer, u64 * length, const cosm_RSA_SIG * sig,
  u64 max_bytes );
  /*
    Save the signature sig into the buffer, writing at most max_bytes bytes.
    length will be set to the number of bytes written.
    Returns: COSM_PASS on success, or an error code on failure.
  */

void CosmRSASigFree( cosm_RSA_SIG * sig );
  /*
    Free all internal data, and zero the signature structure.
    Returns: nothing.
  */

/* signing */

s32 CosmRSAEncode( cosm_RSA_SIG * sig, const cosm_HASH * hash,
  cosmtime timestamp, u8 type, u8 shared, const cosm_RSA_KEY * key );
  /*
    Use the key to encode the hash data, timestamp (in seconds), type, and
    shared flag into the signature sig. A signature of type
    COSM_RSA_SIG_MESSAGE needs a public key, all other types need a private
    key to encode. You should always use COSM_HASH_SHA256 to generate the
    hash for signatures. Care should be taken in chosing the type and
    shared flag you use.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmRSADecode( cosm_HASH * hash, cosmtime * timestamp, u8 * type,
  u8 * shared, const cosm_RSA_SIG * sig, const cosm_RSA_KEY * key );
  /*
    Extract the hash, timestamp, type and shared flags from the signature
    sig using the key. A COSM_PASS from this function does NOT mean the
    signature is valid, only that it was decoded correctly. A signature
    of type COSM_RSA_SIG_MESSAGE needs a private key, all other types need
    a public key to decode.
    It is important to understand that the undecoded parameters of a
    signature (sig.timestamp, sig.type, sig.shared) are MEANINGLESS,
    because they can be trivially modified by an attacker.
    Only use the decoded values.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/* "Keyring" functions */

/* Low level CRC32 functions */
/* 32 bits, for error checking */

s32 Cosm_CRC32Init( cosm_TRANSFORM * transform, va_list params );
  /*
    CRC32 hash transform. 32 bits, only suitable for simple error checks.
    params =  (cosm_HASH *) to the location the hash will be written.
    Returns: COSM_PASS on success, or a transform code on failure.
  */

s32 Cosm_CRC32( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Update hash with the addition of length bytes of data.
    Returns: COSM_PASS on success, or a transform code on failure.
  */

s32 Cosm_CRC32End( cosm_TRANSFORM * transform );
  /*
    Free the temporary data and write the hash result.
    Returns: COSM_PASS on success, or a transform code on failure.
  */

#define COSM_HASH_CRC32 Cosm_CRC32Init, Cosm_CRC32, Cosm_CRC32End

/* Low level MD5 functions */
/* 128 bits, preferred for checksums */

s32 Cosm_MD5Init( cosm_TRANSFORM * transform, va_list params );
  /*
    MD5 hash transform.  128 bits.
    params =  (cosm_HASH *) to the location the hash will be written.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_MD5( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Update hash with the addition of length bytes of data.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_MD5End( cosm_TRANSFORM * transform );
  /*
    Free the temporary data and write the hash result.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_HASH_MD5 Cosm_MD5Init, Cosm_MD5, Cosm_MD5End

/* Low level SHA1 functions */
/* 160 bits, preferred for signing hashes */

s32 Cosm_SHA1Init( cosm_TRANSFORM * transform, va_list params );
  /*
    SHA1 hash transform. 160 bits.
    params =  (cosm_HASH *) to the location the hash will be written.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_SHA1( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Update hash with the addition of length bytes of data.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_SHA1End( cosm_TRANSFORM * transform );
  /*
    Free the temporary data and write the hash result.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_HASH_SHA1 Cosm_SHA1Init, Cosm_SHA1, Cosm_SHA1End

/* Low level MD5+SHA1 functions */
/* 256 bits, preferred for RSA operations */

s32 Cosm_SHA256Init( cosm_TRANSFORM * transform, va_list params );
  /*
    SHA256 hash transform. 256 bits.
    params =  (cosm_HASH *) to the location the hash will be written.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_SHA256( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Update hash with the addition of length bytes of data.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_SHA256End( cosm_TRANSFORM * transform );
  /*
    Free the temporary data and write the hash result.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_HASH_SHA256 Cosm_SHA256Init, Cosm_SHA256, Cosm_SHA256End

/* low level AES/Rijndael API */

s32 Cosm_AESInit( cosm_TRANSFORM * transform, va_list params );
  /*
    Allocate the temporary data and initialize the encryptor.
    params = !!!

    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_AES( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Feed data to the encryption engine, write any results to the buffer..
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_AESEnd( cosm_TRANSFORM * transform );
  /*
    Flush any remaining data and free the temporary data.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

/* Rijndael as implemented to AES specs */
#define COSM_CRYPTO_AES Cosm_AESInit, Cosm_AES, Cosm_AESEnd

/* testing */

s32 Cosm_TestSecurity( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
