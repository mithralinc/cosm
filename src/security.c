/*
  Copyright 1995-2019 Mithral Inc.

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

/*
  This code is exportable under section 740.13(e) (TSU) and section
  740.17(b)(4)(i) (ENC) of the Export Administration Regulations (EAR).
  Mithral Communications & Design, Inc. has notified the Bureau of Export
  Administration (BXA) of the location of this code.
*/

#include "cosm/security.h"
#include "cosm/os_mem.h"
#include "cosm/time.h"
#include "cosm/os_file.h"
#include "cosm/os_io.h"

s32 CosmHashEq( const cosm_HASH * hashA, const cosm_HASH * hashB )
{
  u32 * a;
  u32 * b;
  u32 i;

  a = (u32 *) hashA;
  b = (u32 *) hashB;

  for ( i = 0 ; i < ( sizeof( cosm_HASH ) / 4 ) ; i++ )
  {
    if ( *(a++) != *(b++) )
    {
      return 0;
    }
  }

  return 1;
}

s32 CosmPRNG( cosm_PRNG * prng, void * data, u64 length,
  const void * salt, u64 salt_length )
{
  cosm_TRANSFORM transform;
  cosm_HASH hash;
  u64 bytes;

  if ( ( prng == NULL ) || ( ( data == NULL ) && ( salt == NULL ) ) )
  {
    return COSM_FAIL;
  }

  if ( ( salt != NULL ) && ( salt_length != 0 ) )
  {
    /* add in the salt */
    CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
    CosmTransformInit( &transform, COSM_HASH_MD5, NULL, &hash );
    CosmTransform( &transform, prng, sizeof( cosm_PRNG ) );
    CosmTransform( &transform, salt, salt_length );
    CosmTransformEnd( &transform );
    CosmMemCopy( prng, &hash, sizeof( cosm_PRNG ) );
  }

  if ( ( data != NULL ) && ( length != 0 ) )
  {
    /* generate length bytes of deterministic random bits: */
    while ( length > 0 )
    {
      bytes = ( length > sizeof( cosm_PRNG ) ) ? sizeof( cosm_PRNG ) : length;

      CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
      CosmTransformInit( &transform, COSM_HASH_MD5, NULL, &hash );
      CosmTransform( &transform, prng, sizeof( cosm_PRNG ) );
      CosmTransformEnd( &transform );
      CosmMemCopy( prng, &hash, sizeof( cosm_PRNG ) );

      CosmMemCopy( data, &hash, bytes );
      data = CosmMemOffset( data, bytes );
      length = length - bytes;
    }
  }

  return COSM_PASS;
}

s32 CosmEntropy( u8 * data, u64 length )
{
  return COSM_FAIL;
}

s32 CosmRSAKeyGen( cosm_RSA_KEY * public_key, cosm_RSA_KEY * private_key,
  u32 bits, const u8 * rnd_bits, u64 id, const utf8 * alias,
  cosmtime create, cosmtime expire, void (*callback)( s32, s32, void * ),
  void * callback_param )
{
  cosm_BN p;    /* bits/2, p > q */
  cosm_BN q;    /* bits/2, q < p */
  cosm_BN e;    /* COSM_BN_E */
  cosm_BN d;    /* bits/2 e^(-1) mod (p-1)(q-1) */
  cosm_BN n;    /* bits, p * q */
  cosm_BN dmp1; /* bits/2, d mod (p-1) */
  cosm_BN dmq1; /* bits/2, d mod (q-1) */
  cosm_BN iqmp; /* bits/2, inverse of q mod p */
  cosm_BN x;    /* temp value */
  u8 test_bits[63]; /* enough for a 512 bit key */
  u32 b;
  s32 i;

  if ( ( public_key == NULL ) || ( private_key == NULL ) || ( bits < 512 )
    || ( rnd_bits == NULL ) || ( alias == NULL )
    || ( CosmStrBytes( alias ) > 31 ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* check for bits being a power of 2 */
  b = bits;
  while ( ( b & 1 ) == 0 )
  {
    b >>= 1;
  }
  if ( b != 1 )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  if ( ( CosmBNInit( &p ) != COSM_PASS ) || ( CosmBNInit( &q ) != COSM_PASS )
    || ( CosmBNInit( &e ) != COSM_PASS ) || ( CosmBNInit( &d ) != COSM_PASS )
    || ( CosmBNInit( &n ) != COSM_PASS ) || ( CosmBNInit( &dmp1 ) != COSM_PASS )
    || ( CosmBNInit( &dmq1 ) != COSM_PASS ) || ( CosmBNInit( &iqmp ) != COSM_PASS )
    || ( CosmBNInit( &x ) != COSM_PASS ) )
  {
    return COSM_RSA_ERROR_MEMORY;
  }

  /* generate p and q */

  if ( CosmBNPrimeGen( &p, bits / 2, rnd_bits, callback, callback_param )
    != COSM_PASS )
  {
    CosmBNFree( &p );
    return COSM_RSA_ERROR_MEMORY;
  }

  if ( CosmBNPrimeGen( &q, bits / 2, &rnd_bits[bits/16], callback,
    callback_param ) != COSM_PASS )
  {
    CosmBNFree( &p );
    CosmBNFree( &q );
    return COSM_RSA_ERROR_MEMORY;
  }

  /* make sure p > q */
  i = CosmBNCmp( &p, &q );
  if ( i == -1 )
  {
    /* swap */
    CosmBNSet( &x, &p );
    CosmBNSet( &p, &q );
    CosmBNSet( &q, &x );
  }
  if ( i == 0 )
  {
    /* we said random! */
    CosmBNFree( &p );
    CosmBNFree( &q );
    CosmBNInit( &x );
    return COSM_RSA_ERROR_PARAM;
  }

  if ( /* n = p * q */
    ( CosmBNMul( &n, &p, &q ) != COSM_PASS )
    /* iqmp = inverse of q mod p */
    || ( CosmBNModInv( &iqmp, &q, &p ) != COSM_PASS )
    /* p = p - 1, q = q - 1, both are odd, so just set 0th bit to 0 */
    || ( Cosm_BNBitSet( &p, 0, 0 ) != COSM_PASS )
    || ( Cosm_BNBitSet( &q, 0, 0 ) != COSM_PASS )
    /* d = e^(-1) mod (p-1)(q-1) */
    || ( CosmBNSets32( &e, COSM_BN_E ) != COSM_PASS )
    || ( CosmBNMul( &x, &p, &q ) != COSM_PASS )
    || ( CosmBNModInv( &d, &e, &x ) != COSM_PASS )
    /* dmp1 = d mod (p-1) */
    || ( CosmBNMod( &dmp1, &d, &p ) != COSM_PASS )
    /* dmq1 = d mod (q-1) */
    || ( CosmBNMod( &dmq1, &d, &q ) != COSM_PASS )
    /* get p and q back */
    || ( Cosm_BNBitSet( &p, 0, 1 ) != COSM_PASS )
    || ( Cosm_BNBitSet( &q, 0, 1 ) != COSM_PASS ) )
  {
    CosmBNFree( &p );
    CosmBNFree( &q );
    CosmBNFree( &e );
    CosmBNFree( &d );
    CosmBNFree( &n );
    CosmBNFree( &dmp1 );
    CosmBNFree( &dmq1 );
    CosmBNFree( &iqmp );
    CosmBNFree( &x );
    return COSM_RSA_ERROR_MEMORY;
  }

  /* now we have the numbers, test them */
  if ( callback != NULL )
  {
    (*callback)( 3, i, callback_param );
  }
  if ( ( CosmBNLoad( &x, rnd_bits, 504 ) != COSM_PASS )
    || ( CosmBNModExp( &x, &x, &e, &n ) != COSM_PASS )
    || ( CosmBNModExp( &x, &x, &d, &n ) != COSM_PASS )
    || ( CosmBNSave( test_bits, &x, 504, 504 ) != COSM_PASS )
    || ( CosmMemCmp( rnd_bits, test_bits, 63LL ) != 0 ) )
  {
    CosmBNFree( &p );
    CosmBNFree( &q );
    CosmBNFree( &e );
    CosmBNFree( &d );
    CosmBNFree( &n );
    CosmBNFree( &dmp1 );
    CosmBNFree( &dmq1 );
    CosmBNFree( &iqmp );
    CosmBNFree( &x );
    return COSM_RSA_ERROR_FORMAT;
  }

  if ( callback != NULL )
  {
    (*callback)( 4, i, callback_param );
  }

  /* set headers, make sure extra alias bytes are 0's */
  public_key->pkt_type = COSM_RSA_PUBLIC;
  public_key->pkt_version = COSM_RSA_VERSION;
  public_key->bits = bits;
  public_key->id = id;
  public_key->create = create.hi;
  public_key->expire = expire.hi;
  CosmMemSet( public_key->alias, 32LL, 0 );
  CosmStrCopy( public_key->alias, alias, 32 );

  private_key->pkt_type = COSM_RSA_PRIVATE;
  private_key->pkt_version = COSM_RSA_VERSION;
  private_key->bits = bits;
  private_key->id = id;
  private_key->create = create.hi;
  private_key->expire = expire.hi;
  CosmMemSet( private_key->alias, 32LL, 0 );
  CosmStrCopy( private_key->alias, alias, 32 );

  /* set numbers */
  CosmBNSet( &public_key->n, &n );

  CosmBNSet( &private_key->n, &n );
  CosmBNSet( &private_key->d, &d );
  CosmBNSet( &private_key->p, &p );
  CosmBNSet( &private_key->q, &q );
  CosmBNSet( &private_key->dmp1, &dmp1 );
  CosmBNSet( &private_key->dmq1, &dmq1 );
  CosmBNSet( &private_key->iqmp, &iqmp );

  CosmBNFree( &p );
  CosmBNFree( &q );
  CosmBNFree( &e );
  CosmBNFree( &d );
  CosmBNFree( &n );
  CosmBNFree( &dmp1 );
  CosmBNFree( &dmq1 );
  CosmBNFree( &iqmp );
  CosmBNFree( &x );

  return COSM_PASS;
}

s32 CosmRSAKeyLoad( cosm_RSA_KEY * key, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes, const cosm_HASH * pass_hash )
{
  cosm_TRANSFORM crypt_trans;
  cosm_TRANSFORM hash_trans;
  cosm_TRANSFORM mem_trans;
  cosm_HASH hash;
  u8 * decode;
  u32 size;
  u32 head;

  if ( ( key == NULL ) || ( bytes_read == NULL ) || ( buffer == NULL ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* load header */
  CosmU16Load( &key->pkt_type, &buffer[0] );
  CosmU16Load( &key->pkt_version, &buffer[2] );
  CosmU32Load( &key->bits, &buffer[4] );
  CosmU64Load( (u64 *) &key->create, &buffer[8] );
  CosmU64Load( &key->id, &buffer[16] );
  CosmU64Load( (u64 *) &key->expire, &buffer[24] );
  if ( key->pkt_version == 0 )
  {
    head = 48;
    CosmMemSet( key->alias, 16LL, 0 );
    CosmStrCopy( key->alias, (utf8 *) &buffer[32], 16 );
  }
  else
  {
    head = 64;
    CosmMemSet( key->alias, 32LL, 0 );
    CosmStrCopy( key->alias, (utf8 *) &buffer[32], 32 );
  }

  /* check that what we loaded was a valid key */
  if ( ( ( key->pkt_type != COSM_RSA_PUBLIC ) &&
    ( key->pkt_type != COSM_RSA_PRIVATE ) )
    || ( ( key->pkt_version != 0 )
    && ( key->pkt_version != COSM_RSA_VERSION ) )
    || ( key->bits < 512 ) )
  {
    return COSM_RSA_ERROR_FORMAT;
  }

  /* size is bytes in a half-n, i.e. p and q */
  size = ( key->bits / 16 );

  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    if ( max_bytes < (u64) ( ( size * 2 ) + head ) )
    {
      return COSM_RSA_ERROR_FORMAT;
    }

    if ( CosmBNInit( &key->n ) != COSM_PASS )
    {
      return COSM_RSA_ERROR_MEMORY;
    }

    if ( CosmBNLoad( &key->n, &buffer[head], key->bits ) != COSM_PASS )
    {
      CosmBNFree( &key->n );
      return COSM_RSA_ERROR_MEMORY;
    }

    *bytes_read = (u64) ( ( size * 2 ) + head );
  }
  else
  {
    /* private */
    if ( pass_hash == NULL )
    {
      return COSM_RSA_ERROR_PARAM;
    }

    if ( max_bytes < (u64) ( ( size * 9 ) + head + 20 ) )
    {
      return COSM_RSA_ERROR_FORMAT;
    }

    /* decode memory */
    if ( ( decode = CosmMemAllocSecure( size * 7 ) ) == NULL )
    {
      return COSM_RSA_ERROR_MEMORY;
    }

    /* decrypt */

    CosmMemSet( &mem_trans, sizeof( cosm_TRANSFORM ), 0 );
    CosmMemSet( &crypt_trans, sizeof( cosm_TRANSFORM ), 0 );
    if ( ( CosmTransformInit( &mem_trans, COSM_TRANSFORM_TO_MEMORY,
      NULL, decode ) != COSM_PASS )
      || ( CosmTransformInit( &crypt_trans, COSM_CRYPTO_AES, &mem_trans,
      COSM_CRYPTO_MODE_ECB, COSM_CRYPTO_DECRYPT, pass_hash, 256,
      &buffer[( size * 9 ) + head] ) != COSM_PASS )
      || ( CosmTransform( &crypt_trans, &buffer[( size * 2 ) + head],
      size * 7 ) != COSM_PASS )
      || ( CosmTransformEnd( &crypt_trans ) != COSM_PASS )
      || ( CosmTransformEnd( &mem_trans ) != COSM_PASS ) )
    {
      CosmMemFree( decode );
      CosmTransformEnd( &crypt_trans );
      CosmTransformEnd( &mem_trans );
      return COSM_RSA_ERROR_MEMORY;
    }

    /* now we have the decrypted numbers, do a quick CRC32 to test */

    CosmMemSet( &hash_trans, sizeof( cosm_TRANSFORM ), 0 );
    if ( ( CosmTransformInit( &hash_trans, COSM_HASH_CRC32, NULL, &hash )
      != COSM_PASS )
      || ( CosmTransform( &hash_trans, decode, size * 7 ) != COSM_PASS )
      || ( CosmTransformEnd( &hash_trans ) != COSM_PASS ) )
    {
      CosmMemFree( decode );
      CosmTransformEnd( &hash_trans );
      return COSM_RSA_ERROR_MEMORY;
    }

    /* compare to stored CRC32 */
    if ( CosmMemCmp( &hash, &buffer[( size * 9 ) + head + 16], 4LL ) != 0 )
    {
      CosmMemFree( decode );
      return COSM_RSA_ERROR_PASSPHRASE;
    }

    /* everything is OK, load the numbers */

    if ( ( CosmBNInit( &key->n ) != COSM_PASS )
      || ( CosmBNInit( &key->d ) != COSM_PASS )
      || ( CosmBNInit( &key->p ) != COSM_PASS )
      || ( CosmBNInit( &key->q ) != COSM_PASS )
      || ( CosmBNInit( &key->dmp1 ) != COSM_PASS )
      || ( CosmBNInit( &key->dmq1 ) != COSM_PASS )
      || ( CosmBNInit( &key->iqmp ) != COSM_PASS )
      )
    {
      CosmMemFree( decode );
      return COSM_RSA_ERROR_MEMORY;
    }

    if ( ( CosmBNLoad( &key->n, &buffer[head], key->bits ) != COSM_PASS )
      || ( CosmBNLoad( &key->d, &decode[0], key->bits ) != COSM_PASS )
      || ( CosmBNLoad( &key->p, &decode[size * 2], key->bits / 2 ) != COSM_PASS )
      || ( CosmBNLoad( &key->q, &decode[size * 3], key->bits / 2 ) != COSM_PASS )
      || ( CosmBNLoad( &key->dmp1, &decode[size * 4], key->bits / 2 )
      != COSM_PASS )
      || ( CosmBNLoad( &key->dmq1, &decode[size * 5], key->bits / 2 )
      != COSM_PASS )
      || ( CosmBNLoad( &key->iqmp, &decode[size * 6], key->bits / 2 )
      != COSM_PASS ) )
    {
      CosmMemFree( decode );
      CosmBNFree( &key->n );
      CosmBNFree( &key->d );
      CosmBNFree( &key->p );
      CosmBNFree( &key->q );
      CosmBNFree( &key->dmp1 );
      CosmBNFree( &key->dmq1 );
      CosmBNFree( &key->iqmp );
      return COSM_RSA_ERROR_MEMORY;
    }

    CosmMemFree( decode );
    *bytes_read = (u64) ( ( size * 9 ) + head + 20 );
  }

  return COSM_PASS;
}

s32 CosmRSAKeySave( u8 * buffer, u64 * length, const cosm_RSA_KEY * key,
  const u64 max_bytes, const u8 * rnd_bytes,
  const cosm_HASH * pass_hash )
{
  cosm_TRANSFORM crypt_trans;
  cosm_TRANSFORM hash_trans;
  cosm_TRANSFORM mem_trans;
  cosm_HASH hash;
  u8 * encode;
  u32 size, bits;
  u32 head;

  if ( ( buffer == NULL ) || ( length == NULL ) || ( key == NULL ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* save header */
  CosmU16Save( &buffer[0], &key->pkt_type );
  CosmU16Save( &buffer[2], &key->pkt_version );
  CosmU32Save( &buffer[4], &key->bits );
  CosmU64Save( &buffer[8], (u64 *) &key->create );
  CosmU64Save( &buffer[16], &key->id );
  CosmU64Save( &buffer[24], (u64 *) &key->expire );
  if ( key->pkt_version == 0 )
  {
    head = 48;
    CosmMemSet( &buffer[32], 16LL, 0 );
    CosmStrCopy( (utf8 *) &buffer[32], key->alias, 16 );
  }
  else
  {
    head = 64;
    CosmMemSet( &buffer[32], 32LL, 0 );
    CosmStrCopy( (utf8 *) &buffer[32], key->alias, 32 );
  }

  /* size is bytes in a half-n, i.e. p and q */
  size = key->bits / 16;

  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    if ( max_bytes < (u32) ( ( size * 2 ) + head ) )
    {
      return COSM_RSA_ERROR_FORMAT;
    }

    /* save n */
    if ( CosmBNSave( &buffer[head], &key->n, key->bits, key->bits )
      != COSM_PASS )
    {
      return COSM_RSA_ERROR_MEMORY;
    }

    *length = (u64) ( ( size * 2 ) + head );
  }
  else
  {
    if ( ( rnd_bytes == NULL ) || ( pass_hash == NULL ) )
    {
      return COSM_RSA_ERROR_PARAM;
    }

    if ( max_bytes < (u32) ( ( size * 9 ) + head + 20 ) )
    {
      return COSM_RSA_ERROR_FORMAT;
    }

    /* save n */
    if ( CosmBNSave( &buffer[head], &key->n, key->bits, key->bits ) != COSM_PASS )
    {
      return COSM_RSA_ERROR_MEMORY;
    }

    /* save secret numbers into encode buffer */

    if ( ( encode = CosmMemAllocSecure( size * 7 ) ) == NULL )
    {
      return COSM_RSA_ERROR_MEMORY;
    }

    bits = key->bits / 2;
    if ( ( CosmBNSave( &encode[0], &key->d, bits * 2, bits * 2 ) != COSM_PASS )
      || ( CosmBNSave( &encode[size * 2], &key->p, bits, bits ) != COSM_PASS )
      || ( CosmBNSave( &encode[size * 3], &key->q, bits, bits ) != COSM_PASS )
      || ( CosmBNSave( &encode[size * 4], &key->dmp1, bits, bits ) != COSM_PASS )
      || ( CosmBNSave( &encode[size * 5], &key->dmq1, bits, bits ) != COSM_PASS )
      || ( CosmBNSave( &encode[size * 6], &key->iqmp, bits, bits )
      != COSM_PASS ) )
    {
      CosmMemFree( encode );
      return COSM_RSA_ERROR_MEMORY;
    }

    /* calculate the CRC */
    CosmMemSet( &hash_trans, sizeof( cosm_TRANSFORM ), 0 );
    if ( ( CosmTransformInit( &hash_trans, COSM_HASH_CRC32, NULL, &hash )
      != COSM_PASS )
      || ( CosmTransform( &hash_trans, encode, size * 7 ) != COSM_PASS )
      || ( CosmTransformEnd( &hash_trans ) != COSM_PASS ) )
    {
      CosmMemFree( encode );
      CosmTransformEnd( &hash_trans );
      return COSM_RSA_ERROR_MEMORY;
    }

    /* save the IV and CRC */
    CosmMemCopy( &buffer[( size * 9 ) + head], rnd_bytes, 16LL );
    CosmMemCopy( &buffer[( size * 9 ) + head + 16], &hash, 4LL );

    /* encrypt */

    CosmMemSet( &mem_trans, sizeof( cosm_TRANSFORM ), 0 );
    CosmMemSet( &crypt_trans, sizeof( cosm_TRANSFORM ), 0 );

    if ( ( CosmTransformInit( &mem_trans, COSM_TRANSFORM_TO_MEMORY,
      NULL, &buffer[( size * 2 ) + head] ) != COSM_PASS )
      || ( CosmTransformInit( &crypt_trans, COSM_CRYPTO_AES, &mem_trans,
      COSM_CRYPTO_MODE_ECB, COSM_CRYPTO_ENCRYPT, pass_hash, 256, rnd_bytes )
      != COSM_PASS )
      || ( CosmTransform( &crypt_trans, encode, size * 7 ) != COSM_PASS )
      || ( CosmTransformEnd( &crypt_trans ) != COSM_PASS )
      || ( CosmTransformEnd( &mem_trans ) != COSM_PASS ) )
    {
      CosmMemSet( encode, size * 7, 0 );
      CosmMemFree( encode );
      CosmTransformEnd( &crypt_trans );
      CosmTransformEnd( &mem_trans );
    }

    *length = (u64) ( ( size * 9 ) + head + 20 );
  }

  return COSM_PASS;
}

void CosmRSAKeyFree( cosm_RSA_KEY * key )
{
  if ( key == NULL )
  {
    return;
  }

  CosmBNFree( &key->n );
  CosmBNFree( &key->d );
  CosmBNFree( &key->p );
  CosmBNFree( &key->q );
  CosmBNFree( &key->dmp1 );
  CosmBNFree( &key->dmq1 );
  CosmBNFree( &key->iqmp );

  CosmMemSet( key, sizeof( cosm_RSA_KEY ), 0 );
}

s32 CosmRSASigLoad( cosm_RSA_SIG * sig, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes )
{
  if ( ( sig == NULL ) || ( bytes_read == NULL ) || ( buffer == NULL ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  if ( CosmBNInit( &sig->sig ) != COSM_PASS )
  {
    return COSM_RSA_ERROR_MEMORY;
  }

  CosmU16Load( &sig->pkt_type, &buffer[0] );
  CosmU16Load( &sig->pkt_version, &buffer[2] );
  CosmU32Load( &sig->bits, &buffer[4] );
  CosmU64Load( (u64 *) &sig->create, &buffer[8] );
  CosmU64Load( &sig->id, &buffer[16] );
  CosmU64Load( (u64 *) &sig->timestamp, &buffer[24] );
  sig->type = buffer[32];
  sig->shared = buffer[33];

  /* check that what we loaded was a valid signature */
  if ( ( sig->pkt_type != COSM_RSA_SIGNATURE )
    || ( ( sig->pkt_version != 0 )
    && ( sig->pkt_version != COSM_RSA_VERSION ) )
    || ( sig->bits < 512 )
    || ( ( sig->type != COSM_RSA_SIG_MESSAGE )
    && ( sig->type != COSM_RSA_SIG_SIGN )
    && ( sig->type != COSM_RSA_SIG_KNOWN )
    && ( sig->type != COSM_RSA_SIG_WEAK )
    && ( sig->type != COSM_RSA_SIG_STRONG )
    && ( sig->type != COSM_RSA_SIG_TIMESTAMP )
    && ( sig->type != COSM_RSA_SIG_REVOKE ) )
    || ( ( sig->shared != COSM_RSA_SHARED_NO )
    && ( sig->shared != COSM_RSA_SHARED_YES ) ) )
  {
    return COSM_RSA_ERROR_FORMAT;
  }

  if ( max_bytes < (u64) ( ( sig->bits / 8 ) + 34 ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* now load the signature */
  if ( CosmBNLoad( &sig->sig, &buffer[34], sig->bits ) != COSM_PASS )
  {
    CosmBNFree( &sig->sig );
    return COSM_RSA_ERROR_MEMORY;
  }

  *bytes_read = (u64) ( ( sig->bits / 8 ) + 34 );

  return COSM_PASS;
}

s32 CosmRSASigSave( u8 * buffer, u64 * length, const cosm_RSA_SIG * sig,
  u64 max_bytes )
{
  if ( ( buffer == NULL ) || ( length == NULL ) || ( sig == NULL ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  if ( max_bytes < (u64) ( ( sig->bits / 8 ) + 34 ) )
  {
    return COSM_RSA_ERROR_FORMAT;
  }

  /* save number first for error checking */
  if ( CosmBNSave( &buffer[34], &sig->sig, sig->bits, sig->bits )
    != COSM_PASS )
  {
    return COSM_RSA_ERROR_MEMORY;
  }

  CosmU16Save( &buffer[0], &sig->pkt_type );
  CosmU16Save( &buffer[2], &sig->pkt_version );
  CosmU32Save( &buffer[4], &sig->bits );
  CosmU64Save( &buffer[8], (u64 *) &sig->create );
  CosmU64Save( &buffer[16], &sig->id );
  CosmU64Save( &buffer[24], (u64 *) &sig->timestamp );
  buffer[32] = sig->type;
  buffer[33] = sig->shared;

  *length = (u64) ( ( sig->bits / 8 ) + 34 );

  return COSM_PASS;
}

void CosmRSASigFree( cosm_RSA_SIG * sig )
{
  if ( sig == NULL )
  {
    return;
  }

  CosmBNFree( &sig->sig );

  CosmMemSet( sig, sizeof( cosm_RSA_SIG ), 0 );
}

s32 CosmRSAEncode( cosm_RSA_SIG * sig, const cosm_HASH * hash,
  cosmtime timestamp, u8 type, u8 shared,
  const cosm_RSA_KEY * key )
{
  cosm_BN bn_hash, tmp, tmp2;
  cosm_PRNG rnd;
  cosmtime now;
  u8 * padding;
  u32 pad;

  if ( ( sig == NULL ) || ( hash == NULL ) || ( key == NULL )
    || ( key->bits < 512 ) || ( ( shared != COSM_RSA_SHARED_NO )
    && ( shared != COSM_RSA_SHARED_YES ) ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* check key/type combinations */
  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    if ( type != COSM_RSA_SIG_MESSAGE )
    {
      return COSM_RSA_ERROR_PARAM;
    }
  }
  else if ( key->pkt_type == COSM_RSA_PRIVATE )
  {
    if ( ( type != COSM_RSA_SIG_SIGN ) && ( type != COSM_RSA_SIG_KNOWN )
      && ( type != COSM_RSA_SIG_WEAK ) && ( type != COSM_RSA_SIG_STRONG )
      && ( type != COSM_RSA_SIG_TIMESTAMP ) && ( type != COSM_RSA_SIG_REVOKE ) )
    {
      return COSM_RSA_ERROR_PARAM;
    }
  }
  else
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* is key currently valid? */
  CosmSystemClock( &now );
  if ( ( ( key->create > now.hi ) )
    || ( ( key->expire < now.hi ) ) )
  {
    return COSM_RSA_ERROR_EXPIRED;
  }

  /* is the timestamp in range */
  if ( ( ( key->create > timestamp.hi ) )
    || ( ( key->expire < timestamp.hi ) ) )
  {
    return COSM_RSA_ERROR_EXPIRED;
  }

  /* pad hash */
  pad = ( key->bits / 8 ) - 1 - 42;
  if ( ( padding = CosmMemAlloc( pad + 42LL ) )
    == NULL )
  {
    return COSM_RSA_ERROR_MEMORY;
  }

  /* seed with time and hash, sub-seconds not stored and hash is encrypted */
  CosmPRNG( &rnd, NULL, 0, &timestamp, sizeof( cosmtime ) );
  CosmPRNG( &rnd, NULL, 0, hash, sizeof( cosm_HASH ) );

  /*
    Construct message like this:
    m = random bytes | time | type | share | hash
  */
  CosmPRNG( &rnd, padding, (u64) pad, NULL, 0 );
  CosmU64Save( &padding[pad], (u64 *) &timestamp.hi );
  padding[pad + 8] = type;
  padding[pad + 9] = shared;
  CosmMemCopy( &padding[pad + 10], hash, sizeof( cosm_HASH ) );

  if ( ( CosmBNInit( &bn_hash ) != COSM_PASS ) || ( CosmBNInit( &tmp ) != COSM_PASS )
    || ( CosmBNInit( &tmp2 ) != COSM_PASS ) )
  {
    CosmMemFree( padding );
    return COSM_RSA_ERROR_MEMORY;
  }

  if ( CosmBNLoad( &bn_hash, padding, key->bits - 8 ) != COSM_PASS )
  {
    CosmMemFree( padding );
    return COSM_RSA_ERROR_MEMORY;
  }

  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    /* public... sig = hash ^ COSM_BN_E mod key->n */
    if ( ( CosmBNSets32( &tmp, ( key->pkt_version > 0 ) ? COSM_BN_E : 17 )
      != COSM_PASS )
      || ( CosmBNModExp( &sig->sig, &bn_hash, &tmp, &key->n ) != COSM_PASS ) )
    {
      CosmBNFree( &bn_hash );
      CosmBNFree( &tmp );
      CosmMemFree( padding );
      return COSM_RSA_ERROR_MEMORY;
    }
  }
  else
  {
    /* deal with version 0, which had invalid iqmp's */
    if ( sig->pkt_version == 0 )
    {
      if ( CosmBNModExp( &sig->sig, &bn_hash, &key->d, &key->n )
        != COSM_PASS )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        CosmMemFree( padding );
        return( COSM_RSA_ERROR_MEMORY );
      }
    }
    else
    {
      /*
        Fast method for sig = hash ^ d mod n
        tmp  = hash ^ dmp1 % p
        tmp2 = hash ^ dmq1 % q
        tmp = ( ( tmp - tmp2 ) * iqmp ) % p
        sig = ( tmp * q ) + tmp2
      */
      if ( ( CosmBNMod( &tmp, &bn_hash, &key->p ) != COSM_PASS )
        || ( CosmBNModExp( &tmp, &tmp, &key->dmp1, &key->p ) != COSM_PASS )
        || ( CosmBNMod( &tmp2, &bn_hash, &key->q ) != COSM_PASS )
        || ( CosmBNModExp( &tmp2, &tmp2, &key->dmq1, &key->q ) != COSM_PASS )
        || ( CosmBNSub( &tmp, &tmp, &tmp2 ) != COSM_PASS ) )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        CosmBNFree( &tmp2 );
        CosmMemFree( padding );
        return COSM_RSA_ERROR_MEMORY;
      }

      if ( tmp.neg )
      {
        if ( CosmBNAdd( &tmp, &tmp, &key->p ) != COSM_PASS )
        {
          CosmBNFree( &bn_hash );
          CosmBNFree( &tmp );
          CosmBNFree( &tmp2 );
          CosmMemFree( padding );
          return COSM_RSA_ERROR_MEMORY;
        }
      }

      if ( ( CosmBNMul( &tmp, &tmp, &key->iqmp ) != COSM_PASS )
        || ( CosmBNMod( &tmp, &tmp, &key->p ) != COSM_PASS )
        || ( CosmBNMul( &tmp, &tmp, &key->q ) != COSM_PASS )
        || ( CosmBNAdd( &sig->sig, &tmp, &tmp2 ) != COSM_PASS ) )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        CosmBNFree( &tmp2 );
        CosmMemFree( padding );
        return COSM_RSA_ERROR_MEMORY;
      }
    }
  }

  CosmBNFree( &bn_hash );
  CosmBNFree( &tmp );
  CosmBNFree( &tmp2 );
  CosmMemFree( padding );

  sig->pkt_type = COSM_RSA_SIGNATURE;
  sig->pkt_version = COSM_RSA_VERSION;
  sig->bits = key->bits;
  sig->create = key->create;
  sig->id = key->id;
  sig->timestamp = timestamp.hi;

  sig->type = type;
  sig->shared = shared;

  return COSM_PASS;
}

s32 CosmRSADecode( cosm_HASH * hash, cosmtime * timestamp, u8 * type,
  u8 * shared, const cosm_RSA_SIG * sig, const cosm_RSA_KEY * key )
{
  cosm_BN bn_hash, tmp, tmp2;
  u8 * padding;
  u32 pad;

  if ( ( hash == NULL ) || ( timestamp == NULL ) || ( type == NULL )
    || ( sig == NULL ) || ( key == NULL ) || ( key->bits < 512 ) )
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* check key/type combinations */
  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    if ( ( sig->type != COSM_RSA_SIG_SIGN )
      && ( sig->type != COSM_RSA_SIG_KNOWN )
      && ( sig->type != COSM_RSA_SIG_WEAK )
      && ( sig->type != COSM_RSA_SIG_STRONG )
      && ( sig->type != COSM_RSA_SIG_TIMESTAMP )
      && ( sig->type != COSM_RSA_SIG_REVOKE ) )
    {
      return COSM_RSA_ERROR_PARAM;
    }
  }
  else if ( key->pkt_type == COSM_RSA_PRIVATE )
  {
    if ( sig->type != COSM_RSA_SIG_MESSAGE )
    {
      return COSM_RSA_ERROR_PARAM;
    }
  }
  else
  {
    return COSM_RSA_ERROR_PARAM;
  }

  /* key is the ones used on the sig? */
  if ( ( key->id != sig->id )
    || ( key->create != sig->create )
    || ( key->bits != sig->bits ) )
  {
    return COSM_RSA_ERROR_FORMAT;
  }

  if ( ( CosmBNInit( &bn_hash ) != COSM_PASS ) || ( CosmBNInit( &tmp ) != COSM_PASS )
    || ( CosmBNInit( &tmp2 ) != COSM_PASS ) )
  {
    return COSM_RSA_ERROR_MEMORY;
  }

  if ( key->pkt_type == COSM_RSA_PUBLIC )
  {
    /* public, "hash" = sig->sig ^ COSM_BN_E mod key->n */
    if ( ( CosmBNSets32( &tmp, ( key->pkt_version > 0 ) ? COSM_BN_E : 17 )
      != COSM_PASS )
      || ( CosmBNModExp( &bn_hash, &sig->sig, &tmp, &key->n ) != COSM_PASS ) )
    {
      CosmBNFree( &bn_hash );
      CosmBNFree( &tmp );
      return COSM_RSA_ERROR_MEMORY;
    }
  }
  else
  {
    /* deal with version 0, which had invalid iqmp's */
    if ( sig->pkt_version == 0 )
    {
      if ( CosmBNModExp( &bn_hash, &sig->sig, &key->d, &key->n )
        != COSM_PASS )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        return( COSM_RSA_ERROR_MEMORY );
      }
    }
    else
    {
      /*
        Fast method for hash = sig ^ d mod n
        tmp  = sig ^ dmp1 % p
        tmp2 = sig ^ dmq1 % q
        tmp = ( ( tmp - tmp2 ) * iqmp ) % p
        hash = ( tmp * q ) + tmp2
      */
      if ( ( CosmBNMod( &tmp, &sig->sig, &key->p ) != COSM_PASS )
        || ( CosmBNModExp( &tmp, &tmp, &key->dmp1, &key->p ) != COSM_PASS )
        || ( CosmBNMod( &tmp2, &sig->sig, &key->q ) != COSM_PASS )
        || ( CosmBNModExp( &tmp2, &tmp2, &key->dmq1, &key->q ) != COSM_PASS )
        || ( CosmBNSub( &tmp, &tmp, &tmp2 ) != COSM_PASS ) )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        CosmBNFree( &tmp2 );
        return COSM_RSA_ERROR_MEMORY;
      }

      if ( tmp.neg )
      {
        if ( CosmBNAdd( &tmp, &tmp, &key->p ) != COSM_PASS )
        {
          CosmBNFree( &bn_hash );
          CosmBNFree( &tmp );
          CosmBNFree( &tmp2 );
          return COSM_RSA_ERROR_MEMORY;
        }
      }

      if ( ( CosmBNMul( &tmp, &tmp, &key->iqmp ) != COSM_PASS )
        || ( CosmBNMod( &tmp, &tmp, &key->p ) != COSM_PASS )
        || ( CosmBNMul( &tmp, &tmp, &key->q ) != COSM_PASS )
        || ( CosmBNAdd( &bn_hash, &tmp, &tmp2 ) != COSM_PASS ) )
      {
        CosmBNFree( &bn_hash );
        CosmBNFree( &tmp );
        CosmBNFree( &tmp2 );
        return COSM_RSA_ERROR_MEMORY;
      }
    }
  }

  /* space for our message */
  pad = ( key->bits / 8 ) - 1 - 42;
  if ( ( padding = CosmMemAlloc( (u64) ( key->bits / 8 ) ) ) == NULL )
  {
    CosmBNFree( &bn_hash );
    CosmBNFree( &tmp );
    return COSM_RSA_ERROR_MEMORY;
  }

  if ( CosmBNSave( padding, &bn_hash, key->bits - 8, key->bits - 8 )
    != COSM_PASS )
  {
    CosmBNFree( &bn_hash );
    CosmBNFree( &tmp );
    CosmMemFree( padding );
    return COSM_RSA_ERROR_MEMORY;
  }

  /*
    Reconstruct message like this:
    m = random bytes | time | type | share | hash
  */
  CosmU64Load( (u64 *) &timestamp->hi, &padding[pad] );
  timestamp->lo = 0;
  *type = padding[pad + 8];
  *shared = padding[pad + 9];
  CosmMemCopy( hash, &padding[pad + 10], sizeof( cosm_HASH ) );

  CosmBNFree( &bn_hash );
  CosmBNFree( &tmp );
  CosmMemFree( padding );

  /* check that what we decoded seems valid at all */
  if ( ( ( timestamp->hi < key->create ) )
    || ( ( timestamp->hi > key->expire ) )
    || ( ( *type != COSM_RSA_SIG_MESSAGE )
    && ( *type != COSM_RSA_SIG_SIGN )
    && ( *type != COSM_RSA_SIG_KNOWN )
    && ( *type != COSM_RSA_SIG_WEAK )
    && ( *type != COSM_RSA_SIG_STRONG )
    && ( *type != COSM_RSA_SIG_TIMESTAMP )
    && ( *type != COSM_RSA_SIG_REVOKE ) )
    || ( ( *shared != COSM_RSA_SHARED_NO )
    && ( *shared != COSM_RSA_SHARED_YES ) ) )
  {
    return COSM_RSA_ERROR_FORMAT;
  }

  return COSM_PASS;
}

/* AES/Rijndael API */

/*
  This code is derived directly from the reference code with the following
  changes:
    Only 128-bit blocks (AES) are supported, thus eliminating some code.
    Remove errorchecking done higher up.
    Switch statements -> direct calculations (rounds).
    Reformatting.
*/

#define MAXKC       8
#define MAXROUNDS  14

static u8 Logtable[256] =
{
    0,   0,  25,   1,  50,   2,  26, 198,
   75, 199,  27, 104,  51, 238, 223,   3,
  100,   4, 224,  14,  52, 141, 129, 239,
   76, 113,   8, 200, 248, 105,  28, 193,
  125, 194,  29, 181, 249, 185,  39, 106,
   77, 228, 166, 114, 154, 201,   9, 120,
  101,  47, 138,   5,  33,  15, 225,  36,
   18, 240, 130,  69,  53, 147, 218, 142,
  150, 143, 219, 189,  54, 208, 206, 148,
   19,  92, 210, 241,  64,  70, 131,  56,
  102, 221, 253,  48, 191,   6, 139,  98,
  179,  37, 226, 152,  34, 136, 145,  16,
  126, 110,  72, 195, 163, 182,  30,  66,
   58, 107,  40,  84, 250, 133,  61, 186,
   43, 121,  10,  21, 155, 159,  94, 202,
   78, 212, 172, 229, 243, 115, 167,  87,
  175,  88, 168,  80, 244, 234, 214, 116,
   79, 174, 233, 213, 231, 230, 173, 232,
   44, 215, 117, 122, 235,  22,  11, 245,
   89, 203,  95, 176, 156, 169,  81, 160,
  127,  12, 246, 111,  23, 196,  73, 236,
  216,  67,  31,  45, 164, 118, 123, 183,
  204, 187,  62,  90, 251,  96, 177, 134,
   59,  82, 161, 108, 170,  85,  41, 157,
  151, 178, 135, 144,  97, 190, 220, 252,
  188, 149, 207, 205,  55,  63,  91, 209,
   83,  57, 132,  60,  65, 162, 109,  71,
   20,  42, 158,  93,  86, 242, 211, 171,
   68,  17, 146, 217,  35,  32,  46, 137,
  180, 124, 184,  38, 119, 153, 227, 165,
  103,  74, 237, 222, 197,  49, 254,  24,
   13,  99, 140, 128, 192, 247, 112,   7
};

static u8 Alogtable[256] =
{
    1,   3,   5,  15,  17,  51,  85, 255,
   26,  46, 114, 150, 161, 248,  19,  53,
   95, 225,  56,  72, 216, 115, 149, 164,
  247,   2,   6,  10,  30,  34, 102, 170,
  229,  52,  92, 228,  55,  89, 235,  38,
  106, 190, 217, 112, 144, 171, 230,  49,
   83, 245,   4,  12,  20,  60,  68, 204,
   79, 209, 104, 184, 211, 110, 178, 205,
   76, 212, 103, 169, 224,  59,  77, 215,
   98, 166, 241,   8,  24,  40, 120, 136,
  131, 158, 185, 208, 107, 189, 220, 127,
  129, 152, 179, 206,  73, 219, 118, 154,
  181, 196,  87, 249,  16,  48,  80, 240,
   11,  29,  39, 105, 187, 214,  97, 163,
  254,  25,  43, 125, 135, 146, 173, 236,
   47, 113, 147, 174, 233,  32,  96, 160,
  251,  22,  58,  78, 210, 109, 183, 194,
   93, 231,  50,  86, 250,  21,  63,  65,
  195,  94, 226,  61,  71, 201,  64, 192,
   91, 237,  44, 116, 156, 191, 218, 117,
  159, 186, 213, 100, 172, 239,  42, 126,
  130, 157, 188, 223, 122, 142, 137, 128,
  155, 182, 193,  88, 232,  35, 101, 175,
  234,  37, 111, 177, 200,  67, 197,  84,
  252,  31,  33,  99, 165, 244,   7,   9,
   27,  45, 119, 153, 176, 203,  70, 202,
   69, 207,  74, 222, 121, 139, 134, 145,
  168, 227,  62,  66, 198,  81, 243,  14,
   18,  54,  90, 238,  41, 123, 141, 140,
  143, 138, 133, 148, 167, 242,  13,  23,
   57,  75, 221, 124, 132, 151, 162, 253,
   28,  36, 108, 180, 199,  82, 246,   1
};

static u8 S[256] =
{
   99, 124, 119, 123, 242, 107, 111, 197,
   48,   1, 103,  43, 254, 215, 171, 118,
  202, 130, 201, 125, 250,  89,  71, 240,
  173, 212, 162, 175, 156, 164, 114, 192,
  183, 253, 147,  38,  54,  63, 247, 204,
   52, 165, 229, 241, 113, 216,  49,  21,
    4, 199,  35, 195,  24, 150,   5, 154,
    7,  18, 128, 226, 235,  39, 178, 117,
    9, 131,  44,  26,  27, 110,  90, 160,
   82,  59, 214, 179,  41, 227,  47, 132,
   83, 209,   0, 237,  32, 252, 177,  91,
  106, 203, 190,  57,  74,  76,  88, 207,
  208, 239, 170, 251,  67,  77,  51, 133,
   69, 249,   2, 127,  80,  60, 159, 168,
   81, 163,  64, 143, 146, 157,  56, 245,
  188, 182, 218,  33,  16, 255, 243, 210,
  205,  12,  19, 236,  95, 151,  68,  23,
  196, 167, 126,  61, 100,  93,  25, 115,
   96, 129,  79, 220,  34,  42, 144, 136,
   70, 238, 184,  20, 222,  94,  11, 219,
  224,  50,  58,  10,  73,   6,  36,  92,
  194, 211, 172,  98, 145, 149, 228, 121,
  231, 200,  55, 109, 141, 213,  78, 169,
  108,  86, 244, 234, 101, 122, 174,   8,
  186, 120,  37,  46,  28, 166, 180, 198,
  232, 221, 116,  31,  75, 189, 139, 138,
  112,  62, 181, 102,  72,   3, 246,  14,
   97,  53,  87, 185, 134, 193,  29, 158,
  225, 248, 152,  17, 105, 217, 142, 148,
  155,  30, 135, 233, 206,  85,  40, 223,
  140, 161, 137,  13, 191, 230,  66, 104,
   65, 153,  45,  15, 176,  84, 187,  22
};

static u8 Si[256] =
{
   82,   9, 106, 213,  48,  54, 165,  56,
  191,  64, 163, 158, 129, 243, 215, 251,
  124, 227,  57, 130, 155,  47, 255, 135,
   52, 142,  67,  68, 196, 222, 233, 203,
   84, 123, 148,  50, 166, 194,  35,  61,
  238,  76, 149,  11,  66, 250, 195,  78,
    8,  46, 161, 102,  40, 217,  36, 178,
  118,  91, 162,  73, 109, 139, 209,  37,
  114, 248, 246, 100, 134, 104, 152,  22,
  212, 164,  92, 204,  93, 101, 182, 146,
  108, 112,  72,  80, 253, 237, 185, 218,
   94,  21,  70,  87, 167, 141, 157, 132,
  144, 216, 171,   0, 140, 188, 211,  10,
  247, 228,  88,   5, 184, 179,  69,   6,
  208,  44,  30, 143, 202,  63,  15,   2,
  193, 175, 189,   3,   1,  19, 138, 107,
   58, 145,  17,  65,  79, 103, 220, 234,
  151, 242, 207, 206, 240, 180, 230, 115,
  150, 172, 116,  34, 231, 173,  53, 133,
  226, 249,  55, 232,  28, 117, 223, 110,
   71, 241,  26, 113,  29,  41, 197, 137,
  111, 183,  98,  14, 170,  24, 190,  27,
  252,  86,  62,  75, 198, 210, 121,  32,
  154, 219, 192, 254, 120, 205,  90, 244,
   31, 221, 168,  51, 136,   7, 199,  49,
  177,  18,  16,  89,  39, 128, 236,  95,
   96,  81, 127, 169,  25, 181,  74,  13,
   45, 229, 122, 159, 147, 201, 156, 239,
  160, 224,  59,  77, 174,  42, 245, 176,
  200, 235, 187,  60, 131,  83, 153,  97,
   23,  43,   4, 126, 186, 119, 214,  38,
  225, 105,  20,  99,  85,  33,  12, 125
};

static u32 rcon[30] =
{
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
  0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6,
  0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91
};

static const u8 shifts[4][2] =
{
  { 0, 0 },
  { 1, 3 },
  { 2, 2 },
  { 3, 1 }
};

static u8 mul( u8 a, u8 b )
{
  /* multiply two elements of GF(2^m) */
  if ( a && b )
  {
    return Alogtable[( Logtable[a] + Logtable[b] ) % 255];
  }
  else
  {
    return 0;
  }
}

static void KeyAddition( u8 a[4][4], u8 rk[4][4] )
{
  /* XOR corresponding text input and round key input bytes */
  s32 i, j;

  for ( i = 0 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      a[i][j] ^= rk[i][j];
    }
  }
}

static void ShiftRow( u8 a[4][4], u8 d )
{
  /*
    Row 0 remains unchanged
    the other three rows are shifted a variable amount
  */
  u8 tmp[4];
  s32 i, j;

  for ( i = 1 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      tmp[j] = a[i][(j + shifts[i][d]) % 4];
    }
    for ( j = 0 ; j < 4 ; j++ )
    {
      a[i][j] = tmp[j];
    }
  }
}

static void Substitution( u8 a[4][4], u8 box[256] )
{
  /*
    Replace every byte of the input by the byte at that place
    in the nonlinear S-box
  */
  s32 i, j;

  for ( i = 0 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      a[i][j] = box[a[i][j]];
    }
  }
}

static void MixColumn( u8 a[4][4] )
{
  /* Mix the four bytes of every column in a linear way */
  u8 b[4][4];
  s32 i, j;

  for ( j = 0 ; j < 4 ; j++ )
  {
    for ( i = 0 ; i < 4 ; i++ )
    {
      b[i][j] = (u8) ( mul( 2, a[i][j] ) ^ mul( 3, a[(i + 1) % 4][j] )
        ^ a[(i + 2) % 4][j] ^ a[(i + 3) % 4][j] );
    }
  }

  for ( i = 0 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      a[i][j] = b[i][j];
    }
  }
}

static void InvMixColumn( u8 a[4][4] )
{
  /*
    Mix the four bytes of every column in a linear way
    opposite of Mixcolumn
  */
  u8 b[4][4];
  s32 i, j;

  for ( j = 0 ; j < 4 ; j++ )
  {
    for ( i = 0 ; i < 4 ; i++ )
    {
      b[i][j] = (u8) ( mul( 0xe, a[i][j] ) ^ mul( 0xb, a[(i + 1) % 4][j] )
        ^ mul( 0xd, a[(i + 2) % 4][j] ) ^ mul( 0x9, a[(i + 3) % 4][j] ) );
    }
  }

  for ( i = 0 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      a[i][j] = b[i][j];
    }
  }
}

static s32 rijndaelKeySched( u8 k[4][MAXKC], u32 key_bits,
  u8 W[MAXROUNDS+1][4][4] )
{
  /*
    Calculate the necessary round keys
    The number of calculations depends on key_bits
  */
  u32 kc, rounds, j, t;
  s32 i, rconpointer = 0;
  u8 tk[4][MAXKC];

  kc = ( key_bits >> 5 ); /* 128, 192, 256 -> 4, 6, 8 */
  rounds = kc + 6;       /* 128, 192, 256 -> 10, 12, 14 */

  for ( j = 0 ; j < kc ; j++ )
  {
    for ( i = 0 ; i < 4 ; i++ )
    {
      tk[i][j] = k[i][j];
    }
  }

  t = 0;
  /* copy values into round key array */
  for ( j = 0 ; ( j < kc ) && ( t < ( rounds + 1 ) * 4 ) ; j++, t++ )
  {
    for ( i = 0 ; i < 4 ; i++ )
    {
      W[t / 4][i][t % 4] = tk[i][j];
    }
  }

  while ( t < (rounds+1) * 4 )
  {
    /* while not enough round key material calculated */
    /* calculate new values */
    for ( i = 0 ; i < 4 ; i++ )
    {
      tk[i][0] ^= S[tk[( i + 1 ) % 4][kc - 1]];
    }
    tk[0][0] ^= rcon[rconpointer++];


    if ( kc != 8 )
    {
      for ( j = 1 ; j < kc ; j++ )
      {
        for ( i = 0 ; i < 4 ; i++ )
        {
          tk[i][j] ^= tk[i][j - 1];
        }
      }
    }
    else
    {
      for ( j = 1 ; j < kc / 2 ; j++ )
      {
        for ( i = 0 ; i < 4 ; i++ )
        {
          tk[i][j] ^= tk[i][j - 1];
        }
      }
      for ( i = 0 ; i < 4 ; i++ )
      {
        tk[i][kc / 2] ^= S[tk[i][kc / 2 - 1]];
      }
      for ( j = kc / 2 + 1 ; j < kc ; j++ )
      {
        for ( i = 0; i < 4; i++ )
        {
          tk[i][j] ^= tk[i][j - 1];
        }
      }
    }

    /* copy values into round key array */
    for ( j = 0 ; ( j < kc ) && ( t < (rounds + 1) * 4 ) ; j++, t++ )
    {
      for ( i = 0 ; i < 4 ; i++ )
      {
        W[t / 4][i][t % 4] = tk[i][j];
      }
    }
  }

  return COSM_PASS;
}

static s32 rijndaelEncrypt( u8 a[4][4], u32 key_bits,
  u8 rk[MAXROUNDS+1][4][4] )
{
  /* Encryption of one block */
  u32 r, rounds;

  rounds = ( key_bits >> 5 ) + 6; /* 128, 192, 256 -> 10, 12, 14 */

  /* begin with a key addition */
  KeyAddition( a, rk[0] );

  /* rounds-1 ordinary rounds */
  for ( r = 1 ; r < rounds ; r++ )
  {
    Substitution( a, S );
    ShiftRow( a, 0 );
    MixColumn( a );
    KeyAddition( a, rk[r] );
  }

  /* Last round is special: there is no MixColumn */
  Substitution( a, S );
  ShiftRow( a, 0 );
  KeyAddition( a, rk[rounds] );

  return COSM_PASS;
}

static s32 rijndaelDecrypt( u8 a[4][4], u32 key_bits,
  u8 rk[MAXROUNDS+1][4][4] )
{
  u32 r, rounds;

  rounds = ( key_bits >> 5 ) + 6; /* 128, 192, 256 -> 10, 12, 14 */

  /*
    First the special round:
    without InvMixColumn, with extra KeyAddition
  */
  KeyAddition( a, rk[rounds] );
  Substitution( a, Si );
  ShiftRow( a, 1 );

  /* rounds-1 ordinary rounds */
  for ( r = rounds - 1 ; r > 0 ; r-- )
  {
    KeyAddition( a, rk[r] );
    InvMixColumn( a );
    Substitution( a, Si );
    ShiftRow( a, 1 );
  }

  /* End with the extra key addition */
  KeyAddition( a, rk[0] );

  return COSM_PASS;
}

/* high level AES/Rijndael */

typedef struct cosm_AES_CONTEXT
{
  u8 iv[16];
  u8 chain[16];
  u32 count;
  u32 key_bits;
  u8 key_schedule[MAXROUNDS+1][4][4];
  u32 mode;
  u32 direction;
} cosm_AES_CONTEXT;

s32 Cosm_AESInit( cosm_TRANSFORM * transform, va_list params )
{
  cosm_AES_CONTEXT * context;
  u8 k[4][MAXKC];
  u32 i;
  s32 result;

  /* params */
  u32 mode;
  u32 direction;
  const u8 * key;
  u32 key_bits;
  const u8 * iv;

  mode = va_arg( params, u32 );
  direction = va_arg( params, u32 );
  key = va_arg( params, const u8 * );
  key_bits = va_arg( params, u32 );
  iv = va_arg( params, const u8 * );

  if ( ( key == NULL ) || ( ( key_bits & 0x3F ) != 0 )
    || ( ( key_bits >> 6 ) > 4 ) || ( ( key_bits >> 6 ) < 2 )
    || ( ( mode != COSM_CRYPTO_MODE_CFB ) && ( mode != COSM_CRYPTO_MODE_CBC )
    && ( mode != COSM_CRYPTO_MODE_ECB ) )
    || ( ( mode != COSM_CRYPTO_MODE_ECB ) && ( iv == NULL ) ) )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  /* Try to allocate secure memory if possible */
  if ( ( context = (cosm_AES_CONTEXT *) CosmMemAllocSecure(
    sizeof( cosm_AES_CONTEXT ) ) ) == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  /* load key into k */
  for ( i = 0 ; i < ( key_bits / 8 ) ; i++ )
  {
    k[i % 4][i / 4] = key[i];
  }

  result = rijndaelKeySched( k, key_bits, context->key_schedule );

  /* clear k to not leave copy of key on stack */
  for ( i = 0 ; i < ( key_bits / 8 ) ; i++ )
  {
    k[i % 4][i / 4] = 0;
  }

  if ( result != COSM_PASS )
  {
    CosmMemFree( context );
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  if ( mode != COSM_CRYPTO_MODE_ECB )
  {
    if ( ( mode == COSM_CRYPTO_MODE_CBC )
      && ( direction == COSM_CRYPTO_DECRYPT ) )
    {
      for ( i = 0 ; i < 16 ; i++ )
      {
        /* copy iv into chain for de-CBC */
        context->chain[i] = iv[i];
      }
    }
    else
    {
      for ( i = 0 ; i < 16 ; i++ )
      {
        /* copy iv into iv */
        context->iv[i] = iv[i];
      }
    }
  }

  context->key_bits = key_bits;
  context->mode = mode;
  context->direction = direction;
  transform->tmp_data = context;

  return COSM_PASS;
}

s32 Cosm_AES( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_AES_CONTEXT * context;
  u8 block[4][4];
  u8 jump[16];
  u32 i;
  u32 count;
  const u8 * ptr;
  u8 byte;
  s32 result;

  context = transform->tmp_data;
  ptr = data;

  if ( context->mode == COSM_CRYPTO_MODE_CFB )
  {
    while ( length != 0 )
    {
      /* encrypt */
      for ( i = 0 ; i < 16 ; i++ )
      {
        block[i % 4][i / 4] = context->iv[i];
      }
      rijndaelEncrypt( block, context->key_bits, context->key_schedule );

      /* xor "left" byte */
      byte = (u8) ( block[0][0] ^ *ptr );
      /* we need to feed the data to the next transform if there is one */
      if ( transform->next_transform != NULL )
      {
        if ( ( result = CosmTransform( transform->next_transform,
          &byte, 1LL ) ) != COSM_PASS )
        {
          return result;
        }
      }

      /* shift iv left */
      for ( i = 1 ; i < 15 ; i++ )
      {
        context->iv[i-1] = block[i % 4][i / 4];
      }

      /* replace "right" byte with ct */
      if ( context->direction == COSM_CRYPTO_ENCRYPT )
      {
        context->iv[15] = byte;
      }
      else
      {
        context->iv[15] = *ptr;
      }

      ptr++;
      length--;
    }
  }
  else /* CBC or ECB - need to do a block at a time */
  {
    count = context->count;

    /* check for less then a whole block */
    if ( (u64) count + length < 16LL )
    {
      if ( ( context->mode == COSM_CRYPTO_MODE_ECB )
        || ( context->direction == COSM_CRYPTO_DECRYPT /* CBC Decrypt */ ) )
      {
        for ( i = count ; i < ( count + (u32) length ) ; i++ )
        {
          /* load data to iv */
          context->iv[i] = *ptr++;
        }
      }
      else /* CBC Encrypt */
      {
        for ( i = count ; i < ( count + (u32) length ) ; i++ )
        {
          /* XOR data with iv */
          context->iv[i] ^= *ptr++;
        }
      }
      context->count += (u32) length;
      return COSM_PASS;
    }

    while ( count + length > 15LL )
    {
      /* fill up a the block */
      if ( ( context->mode == COSM_CRYPTO_MODE_ECB )
        || ( context->direction == COSM_CRYPTO_DECRYPT /* CBC Decrypt */ ) )
      {
        for ( i = count ; i < 16 ; i++ )
        {
          /* load data to iv */
          context->iv[i] = *ptr++;
        }
      }
      else /* CBC Encrypt */
      {
        for ( i = count ; i < 16 ; i++ )
        {
          /* XOR data with iv */
          context->iv[i] ^= *ptr++;
        }
      }
      length = ( length - ( 16 - count ) );
      count = 0;

      if ( ( context->mode == COSM_CRYPTO_MODE_CBC )
        && ( context->direction == COSM_CRYPTO_DECRYPT ) )
      {
        for ( i = 0 ; i < 16 ; i++ )
        {
          /* save the input until other side of decrypt */
          jump[i] = context->iv[i];
        }
      }

      /* got a block worth, do crypt */
      for ( i = 0 ; i < 16 ; i++ )
      {
        block[i % 4][i / 4] = context->iv[i];
      }

      if ( context->direction == COSM_CRYPTO_ENCRYPT )
      {
        rijndaelEncrypt( block, context->key_bits, context->key_schedule );
      }
      else
      {
        rijndaelDecrypt( block, context->key_bits, context->key_schedule );
      }

      for ( i = 0 ; i < 16 ; i++ )
      {
        context->iv[i] = block[i % 4][i / 4];
        block[i % 4][i / 4] = 0;
      }

      if ( ( context->mode == COSM_CRYPTO_MODE_CBC )
        && ( context->direction == COSM_CRYPTO_DECRYPT ) )
      {
        for ( i = 0 ; i < 16 ; i++ )
        {
          /* XOR with old block/IV, and set chain for next round */
          context->iv[i] ^= context->chain[i];
          context->chain[i] = jump[i];
          jump[i] = 0;
        }
      }

      /* we need to feed the data to the next transform if there is one */
      if ( transform->next_transform != NULL )
      {
        if ( ( result = CosmTransform( transform->next_transform,
          context->iv, 16LL ) ) != COSM_PASS )
        {
          return result;
        }
      }
    }

    /* put the remaining bytes into the array */
    if ( ( context->mode == COSM_CRYPTO_MODE_ECB )
      || ( context->direction == COSM_CRYPTO_DECRYPT /* CBC Decrypt */ ) )
    {
      for ( i = 0 ; i < (u32) length ; i++ )
      {
        /* load data to iv */
        context->iv[i] = *ptr++;
      }
    }
    else /* CBC Encrypt */
    {
      for ( i = 0 ; i < (u32) length ; i++ )
      {
        /* XOR data with iv */
        context->iv[i] ^= *ptr++;
      }
    }
    context->count = (u32) length;
  }

  return COSM_PASS;
}

s32 Cosm_AESEnd( cosm_TRANSFORM * transform )
{
  cosm_AES_CONTEXT * context;
  u8 padding[16];
  u32 i;

  context = transform->tmp_data;

  if ( ( context->mode != COSM_CRYPTO_MODE_CFB ) && ( context->count != 0 ) )
  {
    /* pad the last block, and feed that padding to the engine */
    for ( i = 0 ; i < ( 15 - context->count ) ; i++ )
    {
      padding[i] = 0;
    }
    padding[15 - context->count] = (u8) ( 16 - context->count );
    Cosm_AES( transform, padding, ( 16 - context->count ) );
  }

  /* output is now flushed */

  /* clear the context */
  CosmMemSet( context, sizeof( cosm_AES_CONTEXT ), 0 );
  CosmMemFree( context );

  return COSM_PASS;
}

/* cleanup AES/Rijndael defines */
#undef MAXKC
#undef MAXROUNDS

/* CRC32 Algoritm */

#define COSM_CRC32_POLY 0xEDB88320 /* Reflected 0x04C11DB7, CCITT */

/* global table, 1 KB */
static const u32 crc32_table[256] =
{
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
  0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
  0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
  0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
  0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
  0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
  0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
  0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
  0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
  0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
  0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
  0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
  0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
  0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
  0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
  0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
  0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
  0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
  0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
  0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
  0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
  0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
  0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
  0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
  0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
  0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
  0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
  0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
  0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
  0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
  0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
  0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
  0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
  0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
  0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
  0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
  0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
  0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
  0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
  0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
  0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
  0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
  0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

typedef struct cosm_CRC32_CONTEXT
{
  u32 current;    /* the CRC so far */
  cosm_HASH * hash;
} cosm_CRC32_CONTEXT;

s32 Cosm_CRC32Init( cosm_TRANSFORM * transform, va_list params )
{
  cosm_CRC32_CONTEXT * context;

  if ( ( context = CosmMemAllocSecure( sizeof( cosm_CRC32_CONTEXT ) ) )
    == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  context->current = 0xFFFFFFFF;

  context->hash = va_arg( params, cosm_HASH * );
  if ( context->hash == NULL )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }
  transform->tmp_data = context;

  return COSM_PASS;
}

s32 Cosm_CRC32( cosm_TRANSFORM * transform, const void * const data,
  u64 length )
{
  cosm_CRC32_CONTEXT * context;
  u32 crc;
  u8 * bytes;

  context = transform->tmp_data;

  crc = context->current;
  bytes = (u8 *) data;

  while ( ( length > 0 ) )
  {
    crc = ( crc >> 8 ) ^ crc32_table[( ( *(bytes++) ^ crc ) ) & 0xFF];
    length--;
  }

  context->current = crc;

  return COSM_PASS;
}

s32 Cosm_CRC32End( cosm_TRANSFORM * transform )
{
  cosm_CRC32_CONTEXT * context;
  s32 i;

  /* Save the CRC */
  context = transform->tmp_data;
  context->current = ~context->current;
  CosmU32Save( &context->hash->hash, &context->current );

  /* make sure we clear the unused hash bytes */
  for ( i = 4; i < 32; i++ )
  {
    context->hash->hash[i] = 0;
  }

  /* clear then free */
  CosmMemSet( context, sizeof( cosm_CRC32_CONTEXT ), 0 );
  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

/* Used in MD5 and SHA */
#define _COSM_ROTL( x, n ) ( ( x << n ) | ( x >> ( 32 - n ) ) )

/* MD5 message-digest algorithm */

typedef struct cosm_MD5_CONTEXT
{
  u8 buffer[64];  /* Data Buffer (Transform input) */
  u32 digest[4];  /* Message digest (Transform output) */
  u64 count;      /* Bits of data that have been hashed */
  cosm_HASH * hash;
} cosm_MD5_CONTEXT;

s32 Cosm_MD5Init( cosm_TRANSFORM * transform, va_list params )
{
  cosm_MD5_CONTEXT * context;

  if ( ( context = CosmMemAllocSecure( sizeof( cosm_MD5_CONTEXT ) ) )
    == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  context->count = 0;

  /* Load magic initialization constants. */
  context->digest[0] = 0x67452301;
  context->digest[1] = 0xEFCDAB89;
  context->digest[2] = 0x98BADCFE;
  context->digest[3] = 0x10325476;

  context->hash = va_arg( params, cosm_HASH * );
  if ( context->hash == NULL )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }
  transform->tmp_data = context;

  return COSM_PASS;
}

/*
  Cosm_MD5Transform - The core transform routine for MD5.
  The core transform routine for MD5. This alters an existing MD5 hash to
  reflect the addition of 16 u32's of new data. Cosm_MD5Update blocks the
  data and converts bytes into u32 for this routine.
  Returns: Nothing.
*/
/* F, G, H and I are basic MD5 functions. */
#define _COSM_MD5_F( x, y, z )  ( ( (x) & (y) ) | ( (~x) & (z) ) )
#define _COSM_MD5_G( x, y, z )  ( ( (x) & (z) ) | ( (y) & (~z) ) )
#define _COSM_MD5_H( x, y, z )  ( (x) ^ (y) ^ (z) )
#define _COSM_MD5_I( x, y, z )  ( (y) ^ ( (x) | (~z) ) )

/* Lots of constants in MD5. */
#define COSM_MD5_CONST11 7
#define COSM_MD5_CONST12 12
#define COSM_MD5_CONST13 17
#define COSM_MD5_CONST14 22
#define COSM_MD5_CONST21 5
#define COSM_MD5_CONST22 9
#define COSM_MD5_CONST23 14
#define COSM_MD5_CONST24 20
#define COSM_MD5_CONST31 4
#define COSM_MD5_CONST32 11
#define COSM_MD5_CONST33 16
#define COSM_MD5_CONST34 23
#define COSM_MD5_CONST41 6
#define COSM_MD5_CONST42 10
#define COSM_MD5_CONST43 15
#define COSM_MD5_CONST44 21

/*
  FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
  Rotation is separate from addition to prevent recomputation.
*/
#define _COSM_MD5_FF( a, b, c, d, x, s, ac ) \
  { \
    a += _COSM_MD5_F( b, c, d ) + x + ac; \
    a = _COSM_ROTL( a, s ); \
    a += b; \
  }
#define _COSM_MD5_GG( a, b, c, d, x, s, ac ) \
  { \
    a += _COSM_MD5_G( b, c, d ) + x + ac; \
    a = _COSM_ROTL( a, s ); \
    a += b; \
  }
#define _COSM_MD5_HH( a, b, c, d, x, s, ac ) \
  { \
    a += _COSM_MD5_H( b, c, d ) + x + ac; \
    a = _COSM_ROTL( a, s ); \
    a += b; \
  }
#define _COSM_MD5_II( a, b, c, d, x, s, ac ) \
  { \
    a += _COSM_MD5_I( b, c, d ) + x + ac; \
    a = _COSM_ROTL( a, s ); \
    a += (b); \
  }

static void Cosm_MD5Transform( cosm_MD5_CONTEXT * context )
{
  u32 a, b, c, d;
  u32 x[16];
  u32 i;
  u8 * p;

  a = context->digest[0];
  b = context->digest[1];
  c = context->digest[2];
  d = context->digest[3];

  /* load the 64 u8's to 16 u32's from little endian */
  for ( i = 0 ; i < 16 ; i++ )
  {
    p = &context->buffer[( i * 4 )];
    x[i] = ( (u32) p[3] << 24 ) | ( (u32) p[2] << 16 ) |
      ( (u32) p[1] << 8 ) | (u32) p[0];
  }

  /* Round 1 */
  _COSM_MD5_FF( a, b, c, d, x[ 0], COSM_MD5_CONST11, 0xD76AA478 ); /* 1 */
  _COSM_MD5_FF( d, a, b, c, x[ 1], COSM_MD5_CONST12, 0xE8C7B756 ); /* 2 */
  _COSM_MD5_FF( c, d, a, b, x[ 2], COSM_MD5_CONST13, 0x242070DB ); /* 3 */
  _COSM_MD5_FF( b, c, d, a, x[ 3], COSM_MD5_CONST14, 0xC1BDCEEE ); /* 4 */
  _COSM_MD5_FF( a, b, c, d, x[ 4], COSM_MD5_CONST11, 0xF57C0FAF ); /* 5 */
  _COSM_MD5_FF( d, a, b, c, x[ 5], COSM_MD5_CONST12, 0x4787C62A ); /* 6 */
  _COSM_MD5_FF( c, d, a, b, x[ 6], COSM_MD5_CONST13, 0xA8304613 ); /* 7 */
  _COSM_MD5_FF( b, c, d, a, x[ 7], COSM_MD5_CONST14, 0xFD469501 ); /* 8 */
  _COSM_MD5_FF( a, b, c, d, x[ 8], COSM_MD5_CONST11, 0x698098D8 ); /* 9 */
  _COSM_MD5_FF( d, a, b, c, x[ 9], COSM_MD5_CONST12, 0x8B44F7AF ); /* 10 */
  _COSM_MD5_FF( c, d, a, b, x[10], COSM_MD5_CONST13, 0xFFFF5BB1 ); /* 11 */
  _COSM_MD5_FF( b, c, d, a, x[11], COSM_MD5_CONST14, 0x895CD7BE ); /* 12 */
  _COSM_MD5_FF( a, b, c, d, x[12], COSM_MD5_CONST11, 0x6B901122 ); /* 13 */
  _COSM_MD5_FF( d, a, b, c, x[13], COSM_MD5_CONST12, 0xFD987193 ); /* 14 */
  _COSM_MD5_FF( c, d, a, b, x[14], COSM_MD5_CONST13, 0xA679438E ); /* 15 */
  _COSM_MD5_FF( b, c, d, a, x[15], COSM_MD5_CONST14, 0x49B40821 ); /* 16 */

  /* Round 2 */
  _COSM_MD5_GG (a, b, c, d, x[ 1], COSM_MD5_CONST21, 0xF61E2562 ); /* 17 */
  _COSM_MD5_GG (d, a, b, c, x[ 6], COSM_MD5_CONST22, 0xC040B340 ); /* 18 */
  _COSM_MD5_GG (c, d, a, b, x[11], COSM_MD5_CONST23, 0x265E5A51 ); /* 19 */
  _COSM_MD5_GG (b, c, d, a, x[ 0], COSM_MD5_CONST24, 0xE9B6C7AA ); /* 20 */
  _COSM_MD5_GG (a, b, c, d, x[ 5], COSM_MD5_CONST21, 0xD62F105D ); /* 21 */
  _COSM_MD5_GG (d, a, b, c, x[10], COSM_MD5_CONST22, 0x02441453 ); /* 22 */
  _COSM_MD5_GG (c, d, a, b, x[15], COSM_MD5_CONST23, 0xD8A1E681 ); /* 23 */
  _COSM_MD5_GG (b, c, d, a, x[ 4], COSM_MD5_CONST24, 0xE7D3FBC8 ); /* 24 */
  _COSM_MD5_GG (a, b, c, d, x[ 9], COSM_MD5_CONST21, 0x21E1CDE6 ); /* 25 */
  _COSM_MD5_GG (d, a, b, c, x[14], COSM_MD5_CONST22, 0xC33707D6 ); /* 26 */
  _COSM_MD5_GG (c, d, a, b, x[ 3], COSM_MD5_CONST23, 0xF4D50D87 ); /* 27 */
  _COSM_MD5_GG (b, c, d, a, x[ 8], COSM_MD5_CONST24, 0x455A14ED ); /* 28 */
  _COSM_MD5_GG (a, b, c, d, x[13], COSM_MD5_CONST21, 0xA9E3E905 ); /* 29 */
  _COSM_MD5_GG (d, a, b, c, x[ 2], COSM_MD5_CONST22, 0xFCEFA3F8 ); /* 30 */
  _COSM_MD5_GG (c, d, a, b, x[ 7], COSM_MD5_CONST23, 0x676F02D9 ); /* 31 */
  _COSM_MD5_GG (b, c, d, a, x[12], COSM_MD5_CONST24, 0x8D2A4C8A ); /* 32 */

  /* Round 3 */
  _COSM_MD5_HH (a, b, c, d, x[ 5], COSM_MD5_CONST31, 0xFFFA3942 ); /* 33 */
  _COSM_MD5_HH (d, a, b, c, x[ 8], COSM_MD5_CONST32, 0x8771F681 ); /* 34 */
  _COSM_MD5_HH (c, d, a, b, x[11], COSM_MD5_CONST33, 0x6D9D6122 ); /* 35 */
  _COSM_MD5_HH (b, c, d, a, x[14], COSM_MD5_CONST34, 0xFDE5380C ); /* 36 */
  _COSM_MD5_HH (a, b, c, d, x[ 1], COSM_MD5_CONST31, 0xA4BEEA44 ); /* 37 */
  _COSM_MD5_HH (d, a, b, c, x[ 4], COSM_MD5_CONST32, 0x4BDECFA9 ); /* 38 */
  _COSM_MD5_HH (c, d, a, b, x[ 7], COSM_MD5_CONST33, 0xF6BB4B60 ); /* 39 */
  _COSM_MD5_HH (b, c, d, a, x[10], COSM_MD5_CONST34, 0xBEBFBC70 ); /* 40 */
  _COSM_MD5_HH (a, b, c, d, x[13], COSM_MD5_CONST31, 0x289B7EC6 ); /* 41 */
  _COSM_MD5_HH (d, a, b, c, x[ 0], COSM_MD5_CONST32, 0xEAA127FA ); /* 42 */
  _COSM_MD5_HH (c, d, a, b, x[ 3], COSM_MD5_CONST33, 0xD4EF3085 ); /* 43 */
  _COSM_MD5_HH (b, c, d, a, x[ 6], COSM_MD5_CONST34, 0x04881D05 ); /* 44 */
  _COSM_MD5_HH (a, b, c, d, x[ 9], COSM_MD5_CONST31, 0xD9D4D039 ); /* 45 */
  _COSM_MD5_HH (d, a, b, c, x[12], COSM_MD5_CONST32, 0xE6DB99E5 ); /* 46 */
  _COSM_MD5_HH (c, d, a, b, x[15], COSM_MD5_CONST33, 0x1FA27CF8 ); /* 47 */
  _COSM_MD5_HH (b, c, d, a, x[ 2], COSM_MD5_CONST34, 0xC4AC5665 ); /* 48 */

  /* Round 4 */
  _COSM_MD5_II (a, b, c, d, x[ 0], COSM_MD5_CONST41, 0xF4292244 ); /* 49 */
  _COSM_MD5_II (d, a, b, c, x[ 7], COSM_MD5_CONST42, 0x432AFF97 ); /* 50 */
  _COSM_MD5_II (c, d, a, b, x[14], COSM_MD5_CONST43, 0xAB9423A7 ); /* 51 */
  _COSM_MD5_II (b, c, d, a, x[ 5], COSM_MD5_CONST44, 0xFC93A039 ); /* 52 */
  _COSM_MD5_II (a, b, c, d, x[12], COSM_MD5_CONST41, 0x655B59C3 ); /* 53 */
  _COSM_MD5_II (d, a, b, c, x[ 3], COSM_MD5_CONST42, 0x8F0CCC92 ); /* 54 */
  _COSM_MD5_II (c, d, a, b, x[10], COSM_MD5_CONST43, 0xFFEFF47D ); /* 55 */
  _COSM_MD5_II (b, c, d, a, x[ 1], COSM_MD5_CONST44, 0x85845DD1 ); /* 56 */
  _COSM_MD5_II (a, b, c, d, x[ 8], COSM_MD5_CONST41, 0x6FA87E4F ); /* 57 */
  _COSM_MD5_II (d, a, b, c, x[15], COSM_MD5_CONST42, 0xFE2CE6E0 ); /* 58 */
  _COSM_MD5_II (c, d, a, b, x[ 6], COSM_MD5_CONST43, 0xA3014314 ); /* 59 */
  _COSM_MD5_II (b, c, d, a, x[13], COSM_MD5_CONST44, 0x4E0811A1 ); /* 60 */
  _COSM_MD5_II (a, b, c, d, x[ 4], COSM_MD5_CONST41, 0xF7537E82 ); /* 61 */
  _COSM_MD5_II (d, a, b, c, x[11], COSM_MD5_CONST42, 0xBD3AF235 ); /* 62 */
  _COSM_MD5_II (c, d, a, b, x[ 2], COSM_MD5_CONST43, 0x2AD7D2BB ); /* 63 */
  _COSM_MD5_II (b, c, d, a, x[ 9], COSM_MD5_CONST44, 0xEB86D391 ); /* 64 */

  context->digest[0] += a;
  context->digest[1] += b;
  context->digest[2] += c;
  context->digest[3] += d;

  /* Zero sensitive information. */
  a = b = c = d = 0;
  CosmMemSet( x, sizeof( x ), 0 );
}

s32 Cosm_MD5( cosm_TRANSFORM * transform, const void * const data,
  u64 length )
{
  u32 bytes;
  u32 offset;
  cosm_MD5_CONTEXT * context;
  u8 * ptr;

  context = transform->tmp_data;
  ptr = (u8 *) data;

  offset = (u32) context->count & 0x3F;
  context->count = ( context->count + length );

  while( ( length > 0 ) )
  {
    bytes = 64 - offset;
    if ( (u64) bytes > length )
      bytes = (u32) length;

    CosmMemCopy( &context->buffer[offset], ptr, (u64) bytes );

    length -= (u64) bytes;
    ptr += bytes;
    offset = ( offset + bytes ) & 0x3F;

    if ( offset == 0 )
    {
      Cosm_MD5Transform( context );
    }
  }

  return COSM_PASS;
}

s32 Cosm_MD5End( cosm_TRANSFORM * transform )
{
  cosm_MD5_CONTEXT * context;
  u64 bit_count;
  u8 pad, bytes[8];
  s32 i, j;

  context = transform->tmp_data;

  bit_count = ( context->count << 3 );
  for ( i = 0 ; i < 8 ; i++ )
  {
    bytes[i] = (u8) bit_count;
    bit_count = ( bit_count >> 8 );
  }

  /* Add 0x80 then pad the data with zeros until 8 bytes remain. */
  pad = 0x80;
  Cosm_MD5( transform, &pad, (u64) 1 );

  pad = 0x00;
  while( ( (u32) context->count & 0x3F ) != 56 )
  {
    Cosm_MD5( transform, &pad, (u64) 1 );
  }

  Cosm_MD5( transform, bytes, (u64) 8 );

  /* Copy the digest into the hash little endian and clear the context */
  for ( i = 0 ; i < 4 ; i++ )
  {
    for ( j = 0 ; j < 4 ; j++ )
    {
      context->hash->hash[( i * 4 ) + j]
        = (u8) ( context->digest[i] >> ( j * 8 ) );
    }
  }
  /* make sure we clear the unused hash bytes */
  for ( i = 16; i < 32; i++ )
  {
    context->hash->hash[i] = 0;
  }

  /* clear then free */
  CosmMemSet( context, sizeof( cosm_MD5_CONTEXT ), 0 );
  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

/* SHA1 - Secure Hash Algorithm */

typedef struct cosm_SHA1_CONTEXT
{
  u8 buffer[64];  /* Data Buffer (Transform input) */
  u32 digest[5];  /* Message digest (Transform output) */
  u64 count;      /* Bits of data that have been hashed */
  cosm_HASH * hash;
} cosm_SHA1_CONTEXT;

s32 Cosm_SHA1Init( cosm_TRANSFORM * transform, va_list params )
{
  cosm_SHA1_CONTEXT * context;

  if ( ( context = CosmMemAllocSecure( sizeof( cosm_SHA1_CONTEXT ) ) )
    == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  context->count = 0;

  /* Load initialization constants. */
  context->digest[0] = 0x67452301;
  context->digest[1] = 0xEFCDAB89;
  context->digest[2] = 0x98BADCFE;
  context->digest[3] = 0x10325476;
  context->digest[4] = 0xC3D2E1F0;

  context->hash = va_arg( params, cosm_HASH * );
  if ( context->hash == NULL )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }
  transform->tmp_data = context;

  return COSM_PASS;
}

/*
  Cosm_SHA1Transform - The core transform routine for SHA1.
*/
#define _COSM_SHA_F1(x, y, z)  ( ( x & y ) | ( ~x & z ) )
#define _COSM_SHA_F2(x, y, z)  ( x ^ y ^ z )
#define _COSM_SHA_F3(x, y, z)  ( ( x & y) | ( x & z ) | ( y & z ) )
#define _COSM_SHA_F4(x, y, z)  ( x ^ y ^ z )
#define COSM_SHA_CONST1  0x5A827999
#define COSM_SHA_CONST2  0x6ED9EBA1
#define COSM_SHA_CONST3  0x8F1BBCDC
#define COSM_SHA_CONST4  0xCA62C1D6

/* The primary SHA hashing macro */
#define _COSM_SHA_FG( n ) \
  { \
    T = _COSM_ROTL( A, 5 ) + _COSM_SHA_F##n( B, C, D ) + E + \
      *WP++ + COSM_SHA_CONST##n; \
    E = D; \
    D = C; \
    C = _COSM_ROTL( B, 30 ); \
    B = A; \
    A = T; \
  }

static void Cosm_SHA1Transform( cosm_SHA1_CONTEXT * context )
{
  u32 i, A, B, C, D, E, T;
  u32 W[80], *WP;

  for ( i = 0; i < 16; i++ )
  {
    CosmU32Load( &W[ i ], &context->buffer[ i * 4 ] );
  }

  for ( i = 16; i < 80; i++ )
  {
    W[ i ] = W[ i - 3 ] ^ W[ i - 8 ] ^ W[ i - 14 ] ^ W[ i - 16 ];
    W[ i ] = _COSM_ROTL( W[ i ], 1 );
  }

  A = context->digest[0];
  B = context->digest[1];
  C = context->digest[2];
  D = context->digest[3];
  E = context->digest[4];
  WP = W;

  for ( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 1 );

  for ( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 2 );

  for ( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 3 );

  for ( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 4 );

  context->digest[0] += A;
  context->digest[1] += B;
  context->digest[2] += C;
  context->digest[3] += D;
  context->digest[4] += E;

  A = B = C = D = E = T = 0;
  CosmMemSet( W, sizeof( W ), 0 );
}

s32 Cosm_SHA1( cosm_TRANSFORM * transform, const void * const data,
  u64 length )
{
  u32 bytes;
  u32 offset;
  cosm_SHA1_CONTEXT * context;
  u8 * ptr;

  context = transform->tmp_data;
  ptr = (u8 *) data;

  offset = (u32) context->count & 0x3F;
  context->count = ( context->count + length );

  while( ( length > 0 ) )
  {
    bytes = 64 - offset;
    if ( (u64) bytes > length )
    {
      bytes = (u32) length;
    }

    CosmMemCopy( &context->buffer[offset], ptr, (u64) bytes );

    length -= (u64) bytes;
    ptr += bytes;
    offset = ( offset + bytes ) & 0x3F;

    if ( offset == 0 )
    {
      Cosm_SHA1Transform( context );
    }
  }

  return COSM_PASS;
}

s32 Cosm_SHA1End( cosm_TRANSFORM * transform )
{
  cosm_SHA1_CONTEXT * context;
  u64 bit_count;
  u8 pad, bits[8];
  s8 i;

  context = transform->tmp_data;

  bit_count = ( context->count << 3 );
  CosmU64Save( bits, &bit_count );

  /* Add 0x80 then pad the data with zeros until 8 bytes remain. */
  pad = 0x80;
  Cosm_SHA1( transform, &pad, (u64) 1 );

  pad = 0x00;
  while( ( (u32) context->count & 0x3F ) != 56 )
  {
    Cosm_SHA1( transform, &pad, (u64) 1 );
  }

  /* Fill those 8 bytes with the length in big-endian form. */
  Cosm_SHA1( transform, bits, (u64) 8 );

  /* Copy the digest into the hash variable and zero the scratch space. */
  for ( i = 0; i < 5; i++ )
  {
    CosmU32Save( &context->hash->hash[i * 4], (u32 *) &context->digest[i] );
  }

  /* make sure we clear the unused hash bytes */
  for ( i = 20; i < 32; i++ )
  {
    context->hash->hash[i] = 0;
  }

  /* clear then free */
  CosmMemSet( context, sizeof( cosm_SHA1_CONTEXT ), 0 );
  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

/* SHA256 - Secure Hash Algorithm */

typedef struct cosm_SHA256_CONTEXT
{
  u8 buffer[64];  /* Data Buffer (Transform input) */
  u32 digest[8];  /* Message digest (Transform output) */
  u64 count;      /* Bits of data that have been hashed */
  cosm_HASH * hash;
} cosm_SHA256_CONTEXT;

s32 Cosm_SHA256Init( cosm_TRANSFORM * transform, va_list params )
{
  cosm_SHA256_CONTEXT * context;

  if ( ( context = CosmMemAllocSecure( sizeof( cosm_SHA256_CONTEXT ) ) )
    == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  context->count = 0;

  /* Load initialization constants */
  context->digest[0] = 0x6A09E667;
  context->digest[1] = 0xBB67AE85;
  context->digest[2] = 0x3C6EF372;
  context->digest[3] = 0xA54FF53A;
  context->digest[4] = 0x510E527F;
  context->digest[5] = 0x9B05688C;
  context->digest[6] = 0x1F83D9AB;
  context->digest[7] = 0x5BE0CD19;

  context->hash = va_arg( params, cosm_HASH * );
  if ( context->hash == NULL )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }
  transform->tmp_data = context;

  return COSM_PASS;
}

/*
  Cosm_SHA256Transform - The core transform routine for SHA256.
*/
#define _ROTR( x, n ) ( ( x >> n ) | ( x << ( 32 - n ) ) )

#define _SUM0( x ) ( _ROTR( x,  2 ) ^ _ROTR( x, 13 ) ^ _ROTR( x, 22 ) )
#define _SUM1( x ) ( _ROTR( x,  6 ) ^ _ROTR( x, 11 ) ^ _ROTR( x, 25 ) )
#define _SIG0( x ) ( _ROTR( x,  7 ) ^ _ROTR( x, 18 ) ^ ( x >>  3 ) )
#define _SIG1( x ) ( _ROTR( x, 17 ) ^ _ROTR( x, 19 ) ^ ( x >> 10 ) )

#define _CH( x, y, z )  ( ( x & y ) ^ ( ~x & z ) )
#define _MAJ( x, y, z ) ( ( x & y ) ^ ( x & z ) ^ ( y & z ) )

#define _R(t) \
  ( W[t] = _SIG1( W[t - 2] ) + W[t - 7] + _SIG0( W[t - 15] ) + W[t - 16] )

#define _P( a, b, c, d, e, f, g, h, x, K ) \
{ \
  temp1 = h + _SUM1( e ) + _CH( e, f, g ) + K + x; \
  temp2 = _SUM0( a ) + _MAJ( a, b, c ); \
  d += temp1; \
  h = temp1 + temp2; \
}

static void Cosm_SHA256Transform( cosm_SHA256_CONTEXT * context )
{
  u32 A, B, C, D, E, F, G, H, temp1, temp2, W[64];
  u32 i;

  for ( i = 0; i < 16; i++ )
  {
    CosmU32Load( &W[i], &context->buffer[i * 4] );
  }

  A = context->digest[0];
  B = context->digest[1];
  C = context->digest[2];
  D = context->digest[3];
  E = context->digest[4];
  F = context->digest[5];
  G = context->digest[6];
  H = context->digest[7];

  _P( A, B, C, D, E, F, G, H, W[0], 0x428A2F98 );
  _P( H, A, B, C, D, E, F, G, W[1], 0x71374491 );
  _P( G, H, A, B, C, D, E, F, W[2], 0xB5C0FBCF );
  _P( F, G, H, A, B, C, D, E, W[3], 0xE9B5DBA5 );
  _P( E, F, G, H, A, B, C, D, W[4], 0x3956C25B );
  _P( D, E, F, G, H, A, B, C, W[5], 0x59F111F1 );
  _P( C, D, E, F, G, H, A, B, W[6], 0x923F82A4 );
  _P( B, C, D, E, F, G, H, A, W[7], 0xAB1C5ED5 );

  _P( A, B, C, D, E, F, G, H, W[8], 0xD807AA98 );
  _P( H, A, B, C, D, E, F, G, W[9], 0x12835B01 );
  _P( G, H, A, B, C, D, E, F, W[10], 0x243185BE );
  _P( F, G, H, A, B, C, D, E, W[11], 0x550C7DC3 );
  _P( E, F, G, H, A, B, C, D, W[12], 0x72BE5D74 );
  _P( D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE );
  _P( C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7 );
  _P( B, C, D, E, F, G, H, A, W[15], 0xC19BF174 );

  _P( A, B, C, D, E, F, G, H, _R(16), 0xE49B69C1 );
  _P( H, A, B, C, D, E, F, G, _R(17), 0xEFBE4786 );
  _P( G, H, A, B, C, D, E, F, _R(18), 0x0FC19DC6 );
  _P( F, G, H, A, B, C, D, E, _R(19), 0x240CA1CC );
  _P( E, F, G, H, A, B, C, D, _R(20), 0x2DE92C6F );
  _P( D, E, F, G, H, A, B, C, _R(21), 0x4A7484AA );
  _P( C, D, E, F, G, H, A, B, _R(22), 0x5CB0A9DC );
  _P( B, C, D, E, F, G, H, A, _R(23), 0x76F988DA );

  _P( A, B, C, D, E, F, G, H, _R(24), 0x983E5152 );
  _P( H, A, B, C, D, E, F, G, _R(25), 0xA831C66D );
  _P( G, H, A, B, C, D, E, F, _R(26), 0xB00327C8 );
  _P( F, G, H, A, B, C, D, E, _R(27), 0xBF597FC7 );
  _P( E, F, G, H, A, B, C, D, _R(28), 0xC6E00BF3 );
  _P( D, E, F, G, H, A, B, C, _R(29), 0xD5A79147 );
  _P( C, D, E, F, G, H, A, B, _R(30), 0x06CA6351 );
  _P( B, C, D, E, F, G, H, A, _R(31), 0x14292967 );

  _P( A, B, C, D, E, F, G, H, _R(32), 0x27B70A85 );
  _P( H, A, B, C, D, E, F, G, _R(33), 0x2E1B2138 );
  _P( G, H, A, B, C, D, E, F, _R(34), 0x4D2C6DFC );
  _P( F, G, H, A, B, C, D, E, _R(35), 0x53380D13 );
  _P( E, F, G, H, A, B, C, D, _R(36), 0x650A7354 );
  _P( D, E, F, G, H, A, B, C, _R(37), 0x766A0ABB );
  _P( C, D, E, F, G, H, A, B, _R(38), 0x81C2C92E );
  _P( B, C, D, E, F, G, H, A, _R(39), 0x92722C85 );

  _P( A, B, C, D, E, F, G, H, _R(40), 0xA2BFE8A1 );
  _P( H, A, B, C, D, E, F, G, _R(41), 0xA81A664B );
  _P( G, H, A, B, C, D, E, F, _R(42), 0xC24B8B70 );
  _P( F, G, H, A, B, C, D, E, _R(43), 0xC76C51A3 );
  _P( E, F, G, H, A, B, C, D, _R(44), 0xD192E819 );
  _P( D, E, F, G, H, A, B, C, _R(45), 0xD6990624 );
  _P( C, D, E, F, G, H, A, B, _R(46), 0xF40E3585 );
  _P( B, C, D, E, F, G, H, A, _R(47), 0x106AA070 );

  _P( A, B, C, D, E, F, G, H, _R(48), 0x19A4C116 );
  _P( H, A, B, C, D, E, F, G, _R(49), 0x1E376C08 );
  _P( G, H, A, B, C, D, E, F, _R(50), 0x2748774C );
  _P( F, G, H, A, B, C, D, E, _R(51), 0x34B0BCB5 );
  _P( E, F, G, H, A, B, C, D, _R(52), 0x391C0CB3 );
  _P( D, E, F, G, H, A, B, C, _R(53), 0x4ED8AA4A );
  _P( C, D, E, F, G, H, A, B, _R(54), 0x5B9CCA4F );
  _P( B, C, D, E, F, G, H, A, _R(55), 0x682E6FF3 );

  _P( A, B, C, D, E, F, G, H, _R(56), 0x748F82EE );
  _P( H, A, B, C, D, E, F, G, _R(57), 0x78A5636F );
  _P( G, H, A, B, C, D, E, F, _R(58), 0x84C87814 );
  _P( F, G, H, A, B, C, D, E, _R(59), 0x8CC70208 );
  _P( E, F, G, H, A, B, C, D, _R(60), 0x90BEFFFA );
  _P( D, E, F, G, H, A, B, C, _R(61), 0xA4506CEB );
  _P( C, D, E, F, G, H, A, B, _R(62), 0xBEF9A3F7 );
  _P( B, C, D, E, F, G, H, A, _R(63), 0xC67178F2 );

  context->digest[0] += A;
  context->digest[1] += B;
  context->digest[2] += C;
  context->digest[3] += D;
  context->digest[4] += E;
  context->digest[5] += F;
  context->digest[6] += G;
  context->digest[7] += H;

  A = B = C = D = E = F = G = H = temp1 = temp2 = 0;
  CosmMemSet( W, sizeof( W ), 0 );
}

s32 Cosm_SHA256( cosm_TRANSFORM * transform, const void * const data,
  u64 length )
{
  u32 bytes;
  u32 offset;
  cosm_SHA256_CONTEXT * context;
  u8 * ptr;

  context = transform->tmp_data;
  ptr = (u8 *) data;

  offset = (u32) context->count & 0x3F;
  context->count = ( context->count + length );

  while( ( length > 0 ) )
  {
    bytes = 64 - offset;
    if ( (u64) bytes > length )
    {
      bytes = (u32) length;
    }

    CosmMemCopy( &context->buffer[offset], ptr, (u64) bytes );

    length -= (u64) bytes;
    ptr += bytes;
    offset = ( offset + bytes ) & 0x3F;

    if ( offset == 0 )
    {
      Cosm_SHA256Transform( context );
    }
  }

  return COSM_PASS;
}

s32 Cosm_SHA256End( cosm_TRANSFORM * transform )
{
  cosm_SHA256_CONTEXT * context;
  u64 bit_count;
  u8 pad, bits[8];
  s8 i;

  context = transform->tmp_data;

  bit_count = ( context->count << 3 );
  CosmU64Save( bits, &bit_count );

  /* Add 0x80 then pad the data with zeros until 8 bytes remain. */
  pad = 0x80;
  Cosm_SHA256( transform, &pad, (u64) 1 );

  pad = 0x00;
  while( ( (u32) context->count & 0x3F ) != 56 )
  {
    Cosm_SHA256( transform, &pad, (u64) 1 );
  }

  /* Fill those 8 bytes with the length in big-endian form. */
  Cosm_SHA256( transform, bits, (u64) 8 );

  /* Copy the digest into the hash variable and zero the scratch space. */
  for ( i = 0; i < 8; i++ )
  {
    CosmU32Save( &context->hash->hash[i * 4], (u32 *) &context->digest[i] );
  }

  /* clear then free */
  CosmMemSet( context, sizeof( cosm_SHA256_CONTEXT ), 0 );
  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

/* testing */

s32 Cosm_TestSecurity( void )
{
  const utf8 * str1 = "abc";
  const utf8 * str2 = "message digest";
  cosm_TRANSFORM transform;
  cosm_HASH hash;
  s32 error;

  const cosm_HASH hash_str1_md5 =
  {
    {
      0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
      0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  };
  const cosm_HASH hash_str2_md5 =
  {
    {
      0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
      0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  };
  const cosm_HASH hash_str1_sha1 =
  {
    {
      0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A,
      0xBA, 0x3E, 0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C,
      0x9C, 0xD0, 0xD8, 0x9D, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  };
  const cosm_HASH hash_str2_sha1 =
  {
    {
      0xC1, 0x22, 0x52, 0xCE, 0xDA, 0x8B, 0xe8, 0x99,
      0x4D, 0x5F, 0xA0, 0x29, 0x0A, 0x47, 0x23, 0x1C,
      0x1D, 0x16, 0xAA, 0xE3, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  };
  const cosm_HASH hash_str1_sha256 =
  {
    {
      0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
      0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
      0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
      0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    }
  };
  const cosm_HASH hash_str2_sha256 =
  {
    {
      0xf7, 0x84, 0x6f, 0x55, 0xcf, 0x23, 0xe1, 0x4e,
      0xeb, 0xea, 0xb5, 0xb4, 0xe1, 0x55, 0x0c, 0xad,
      0x5b, 0x50, 0x9e, 0x33, 0x48, 0xfb, 0xc4, 0xef,
      0xa3, 0xa1, 0x41, 0x3d, 0x39, 0x3c, 0xb6, 0x50
    }
  };
  const cosm_HASH hash_crc32 =
  {
    {
      0xCB, 0xF4, 0x39, 0x26, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  };

  cosm_BUFFER buffer;
  cosm_TRANSFORM buf_trans;
  u8 key[32] =
  {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
  };
  u8 iv[16] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  u8 pt[32] =
  {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
  };
  u8 ecb[32] =
  {
    0x0A, 0x94, 0x0B, 0xB5, 0x41, 0x6E, 0xF0, 0x45,
    0xF1, 0xC3, 0x94, 0x58, 0xC6, 0x53, 0xEA, 0x5A,
    0xC8, 0x44, 0x62, 0x9C, 0xE0, 0x1C, 0xF3, 0x93,
    0xC4, 0xE1, 0xEC, 0x53, 0x79, 0x48, 0xD5, 0xA5
  };
  u8 cbc[32] =
  {
    0x0A, 0x94, 0x0B, 0xB5, 0x41, 0x6E, 0xF0, 0x45,
    0xF1, 0xC3, 0x94, 0x58, 0xC6, 0x53, 0xEA, 0x5A,
    0x2F, 0x50, 0x2E, 0x95, 0xFF, 0xD4, 0xC4, 0x0F,
    0xDA, 0x28, 0x52, 0xE0, 0xD9, 0x78, 0x90, 0x28
  };
  u8 cfb[19] =
  {
    0xC6, 0x9A, 0x0B, 0x5A, 0xA0, 0xDC, 0x45, 0x3D,
    0x28, 0xFA, 0x61, 0xDE, 0xB9, 0x9F, 0x90, 0xFD,
    0x79, 0x17, 0x10
  };
  u8 ct[64];
  u8 pt2[64];

  /* MD5 str1 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_MD5, NULL, &hash )
    != COSM_PASS )
  {
    error = -1;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str1, CosmStrBytes( str1 ) ) != COSM_PASS )
  {
    error = -2;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -3;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str1_md5 ) )
  {
    error = -4;
    goto test_failed;
  }

  /* MD5 str2 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_MD5, NULL, &hash )
    != COSM_PASS )
  {
    error = -5;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str2, CosmStrBytes( str2 ) ) != COSM_PASS )
  {
    error = -6;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -7;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str2_md5 ) )
  {
    error = -8;
    goto test_failed;
  }

  /* SHA1 str1 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_SHA1, NULL, &hash )
    != COSM_PASS )
  {
    error = -9;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str1, CosmStrBytes( str1 ) ) != COSM_PASS )
  {
    error = -10;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -11;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str1_sha1 ) )
  {
    error = -12;
    goto test_failed;
  }

  /* SHA1 str2 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_SHA1, NULL, &hash )
    != COSM_PASS )
  {
    error = -13;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str2, CosmStrBytes( str2 ) ) != COSM_PASS )
  {
    error = -14;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -15;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str2_sha1 ) )
  {
    error = -16;
    goto test_failed;
  }

  /* SHA256 str1 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_SHA256, NULL, &hash )
    != COSM_PASS )
  {
    error = -17;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str1, CosmStrBytes( str1 ) ) != COSM_PASS )
  {
    error = -18;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -19;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str1_sha256 ) )
  {
    error = -20;
    goto test_failed;
  }

  /* SHA256 str2 */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_SHA256, NULL, &hash )
    != COSM_PASS )
  {
    error = -21;
    goto test_failed;
  }

  if ( CosmTransform( &transform, str2, CosmStrBytes( str2 ) ) != COSM_PASS )
  {
    error = -22;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -23;
    goto test_failed;;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_str2_sha256 ) )
  {
    error = -24;
    goto test_failed;
  }

  /* CRC32 test */

  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_CRC32, NULL, &hash )
    != COSM_PASS )
  {
    error = -25;
    goto test_failed;
  }

  if ( CosmTransform( &transform, "123456789", (u64) 9 ) != COSM_PASS )
  {
    error = -26;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -27;
    goto test_failed;
  }

  /* Compare the hash */
  if( !CosmHashEq( &hash, &hash_crc32 ) )
  {
    error = -28;
    goto test_failed;
  }

  /* Crypto tests, errors start at -1000 */
  CosmMemSet( &buffer, sizeof( cosm_BUFFER ), 0 );
  CosmMemSet( &transform, sizeof( cosm_TRANSFORM ), 0 );
  CosmMemSet( &buf_trans, sizeof( cosm_TRANSFORM ), 0 );

  /* need a buffer to dump into */
  CosmBufferInit( &buffer, 128, COSM_BUFFER_MODE_QUEUE, 128, NULL, 0 );
  if ( CosmTransformInit( &buf_trans, COSM_TRANSFORM_TO_BUFFER,
    NULL, &buffer ) != COSM_PASS )
  {
    return -1000;
  }

  /* Encrypt ECB */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_ECB, COSM_CRYPTO_ENCRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1001;
    goto test_failed;
  }

  if ( CosmTransform( &transform, pt, 24LL ) != COSM_PASS )
  {
    error = -1002;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1003;
    goto test_failed;
  }

  if ( CosmBufferGet( ct, 64LL, &buffer ) != 32LL )
  {
    error = -1004;
    goto test_failed;
  }

  if ( CosmMemCmp( ct, ecb, 32LL ) != 0 )
  {
    error = -1005;
    goto test_failed;
  }

  /* Decrypt ECB */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_ECB, COSM_CRYPTO_DECRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1006;
    goto test_failed;
  }

  /* need a multiple of 16 bytes */
  if ( CosmTransform( &transform, ct, 32LL ) != COSM_PASS )
  {
    error = -1007;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1008;
    goto test_failed;
  }

  if ( CosmBufferGet( pt2, 64LL, &buffer ) != 32LL )
  {
    error = -1009;
    goto test_failed;
  }

  /* only compare 24 */
  if ( CosmMemCmp( pt, pt2, 24LL ) != 0 )
  {
    error = -1010;
    goto test_failed;
  }

  /* Encrypt CBC */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_CBC, COSM_CRYPTO_ENCRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1011;
    goto test_failed;
  }

  if ( CosmTransform( &transform, pt, 24LL ) != COSM_PASS )
  {
    error = -1012;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1013;
    goto test_failed;
  }

  if ( CosmBufferGet( ct, 64LL, &buffer ) != 32LL )
  {
    error = -1014;
    goto test_failed;
  }

  if ( CosmMemCmp( ct, cbc, 32LL ) != 0 )
  {
    error = -1015;
    goto test_failed;
  }

  /* Decrypt CBC */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_CBC, COSM_CRYPTO_DECRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1016;
    goto test_failed;
  }

  /* need a multiple of 16 bytes */
  if ( CosmTransform( &transform, ct, 32LL ) != COSM_PASS )
  {
    error = -1017;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1018;
    goto test_failed;
  }

  if ( CosmBufferGet( pt2, 64LL, &buffer ) != 32LL )
  {
    error = -1019;
    goto test_failed;
  }

  /* only compare 24 */
  if ( CosmMemCmp( pt, pt2, 24LL ) != 0 )
  {
    error = -1020;
    goto test_failed;
  }

  /* Encrypt CFB */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_CFB, COSM_CRYPTO_ENCRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1021;
    goto test_failed;
  }

  if ( CosmTransform( &transform, pt, 19LL ) != COSM_PASS )
  {
    error = -1022;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1023;
    goto test_failed;
  }

  if ( CosmBufferGet( ct, 64LL, &buffer ) != 19LL )
  {
    error = -1024;
    goto test_failed;
  }

  if ( CosmMemCmp( ct, cfb, 19LL ) != 0 )
  {
    error = -1025;
    goto test_failed;
  }

  /* Decrypt ECB */

  if ( CosmTransformInit( &transform, COSM_CRYPTO_AES, &buf_trans,
    COSM_CRYPTO_MODE_CFB, COSM_CRYPTO_DECRYPT, key, 128, iv ) != COSM_PASS )
  {
    error = -1026;
    goto test_failed;
  }

  if ( CosmTransform( &transform, ct, 19LL ) != COSM_PASS )
  {
    error = -1027;
    goto test_failed;
  }

  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    error = -1028;
    goto test_failed;
  }

  if ( CosmBufferGet( pt2, 64LL, &buffer ) != 19LL )
  {
    error = -1029;
    goto test_failed;
  }

  if ( CosmMemCmp( pt, pt2, 19LL ) != 0 )
  {
    error = -1030;
    goto test_failed;
  }

  CosmTransformEnd( &buf_trans );
  CosmBufferFree( &buffer );

  return COSM_PASS;

test_failed:
  CosmTransformEnd( &transform );
  CosmTransformEnd( &buf_trans );
  CosmBufferFree( &buffer );
  return error;
}
