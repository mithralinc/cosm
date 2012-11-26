/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

/*
  This code is exportable under section 740.13(e) (TSU) [for use under
  Mithral License A], and section 740.17(b)(4)(i) (ENC) [for use under
  Mithral License B] of the Export Administration Regulations (EAR).
  Mithral Communications & Design, Inc. has notified the Bureau of Export
  Administration (BXA) of the location of this code.
*/

#ifndef COSM_CRYPT_C_INC
#error "crypto.c is included directly from security.c, dont compile it alone"
#endif

#ifndef COSM_CRYPTO_C
#define COSM_CRYPTO_C

s32 CosmPKIKeyGen( cosm_PKI_KEY * public_key, cosm_PKI_KEY * private_key,
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
    return COSM_PKI_ERROR_PARAM;
  }

  /* check for bits being a power of 2 */
  b = bits;
  while ( ( b & 1 ) == 0 )
  {
    b >>= 1;
  }
  if ( b != 1 )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  if ( ( CosmBNInit( &p ) != COSM_PASS ) || ( CosmBNInit( &q ) != COSM_PASS )
    || ( CosmBNInit( &e ) != COSM_PASS ) || ( CosmBNInit( &d ) != COSM_PASS )
    || ( CosmBNInit( &n ) != COSM_PASS ) || ( CosmBNInit( &dmp1 ) != COSM_PASS )
    || ( CosmBNInit( &dmq1 ) != COSM_PASS ) || ( CosmBNInit( &iqmp ) != COSM_PASS )
    || ( CosmBNInit( &x ) != COSM_PASS ) )
  {
    return COSM_PKI_ERROR_MEMORY;
  }

  /* generate p and q */

  if ( CosmBNPrimeGen( &p, bits / 2, rnd_bits, callback, callback_param )
    != COSM_PASS )
  {
    CosmBNFree( &p );
    return COSM_PKI_ERROR_MEMORY;
  }

  if ( CosmBNPrimeGen( &q, bits / 2, &rnd_bits[bits/16], callback,
    callback_param ) != COSM_PASS )
  {
    CosmBNFree( &p );
    CosmBNFree( &q );
    return COSM_PKI_ERROR_MEMORY;
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
    return COSM_PKI_ERROR_PARAM;
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
    return COSM_PKI_ERROR_MEMORY;
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
    return COSM_PKI_ERROR_FORMAT;
  }

  if ( callback != NULL )
  {
    (*callback)( 4, i, callback_param );
  }

  /* set headers, make sure extra alias bytes are 0's */
  public_key->pkt_type = COSM_PKI_PUBLIC;
  public_key->pkt_version = COSM_PKI_VERSION;
  public_key->bits = bits;
  public_key->id = id;
  public_key->create = create.hi;
  public_key->expire = expire.hi;
  CosmMemSet( public_key->alias, 32LL, 0 );
  CosmStrCopy( public_key->alias, alias, 32 );

  private_key->pkt_type = COSM_PKI_PRIVATE;
  private_key->pkt_version = COSM_PKI_VERSION;
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

s32 CosmPKIKeyLoad( cosm_PKI_KEY * key, u64 * bytes_read,
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
    return COSM_PKI_ERROR_PARAM;
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
  if ( ( ( key->pkt_type != COSM_PKI_PUBLIC ) &&
    ( key->pkt_type != COSM_PKI_PRIVATE ) )
    || ( ( key->pkt_version != 0 )
    && ( key->pkt_version != COSM_PKI_VERSION ) )
    || ( key->bits < 512 ) )
  {
    return COSM_PKI_ERROR_FORMAT;
  }

  /* size is bytes in a half-n, i.e. p and q */
  size = ( key->bits / 16 );

  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    if ( max_bytes < (u64) ( ( size * 2 ) + head ) )
    {
      return COSM_PKI_ERROR_FORMAT;
    }

    if ( CosmBNInit( &key->n ) != COSM_PASS )
    {
      return COSM_PKI_ERROR_MEMORY;
    }

    if ( CosmBNLoad( &key->n, &buffer[head], key->bits ) != COSM_PASS )
    {
      CosmBNFree( &key->n );
      return COSM_PKI_ERROR_MEMORY;
    }

    *bytes_read = (u64) ( ( size * 2 ) + head );
  }
  else
  {
    /* private */
    if ( pass_hash == NULL )
    {
      return COSM_PKI_ERROR_PARAM;
    }

    if ( max_bytes < (u64) ( ( size * 9 ) + head + 20 ) )
    {
      return COSM_PKI_ERROR_FORMAT;
    }

    /* decode memory */
    if ( ( decode = CosmMemAllocSecure( size * 7 ) ) == NULL )
    {
      return COSM_PKI_ERROR_MEMORY;
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
      return COSM_PKI_ERROR_MEMORY;
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
      return COSM_PKI_ERROR_MEMORY;
    }

    /* compare to stored CRC32 */
    if ( CosmMemCmp( &hash, &buffer[( size * 9 ) + head + 16], 4LL ) != 0 )
    {
      CosmMemFree( decode );
      return COSM_PKI_ERROR_PASSPHRASE;
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
      return COSM_PKI_ERROR_MEMORY;
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
      return COSM_PKI_ERROR_MEMORY;
    }

    CosmMemFree( decode );
    *bytes_read = (u64) ( ( size * 9 ) + head + 20 );
  }

  return COSM_PASS;
}

s32 CosmPKIKeySave( u8 * buffer, u64 * length, const cosm_PKI_KEY * key,
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
    return COSM_PKI_ERROR_PARAM;
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

  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    if ( max_bytes < (u32) ( ( size * 2 ) + head ) )
    {
      return COSM_PKI_ERROR_FORMAT;
    }

    /* save n */
    if ( CosmBNSave( &buffer[head], &key->n, key->bits, key->bits )
      != COSM_PASS )
    {
      return COSM_PKI_ERROR_MEMORY;
    }

    *length = (u64) ( ( size * 2 ) + head );
  }
  else
  {
    if ( ( rnd_bytes == NULL ) || ( pass_hash == NULL ) )
    {
      return COSM_PKI_ERROR_PARAM;
    }

    if ( max_bytes < (u32) ( ( size * 9 ) + head + 20 ) )
    {
      return COSM_PKI_ERROR_FORMAT;
    }

    /* save n */
    if ( CosmBNSave( &buffer[head], &key->n, key->bits, key->bits ) != COSM_PASS )
    {
      return COSM_PKI_ERROR_MEMORY;
    }

    /* save secret numbers into encode buffer */

    if ( ( encode = CosmMemAllocSecure( size * 7 ) ) == NULL )
    {
      return COSM_PKI_ERROR_MEMORY;
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
      return COSM_PKI_ERROR_MEMORY;
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
      return COSM_PKI_ERROR_MEMORY;
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

void CosmPKIKeyFree( cosm_PKI_KEY * key )
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

  CosmMemSet( key, sizeof( cosm_PKI_KEY ), 0 );
}

s32 CosmPKISigLoad( cosm_PKI_SIG * sig, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes )
{
  if ( ( sig == NULL ) || ( bytes_read == NULL ) || ( buffer == NULL ) )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  if ( CosmBNInit( &sig->sig ) != COSM_PASS )
  {
    return COSM_PKI_ERROR_MEMORY;
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
  if ( ( sig->pkt_type != COSM_PKI_SIGNATURE )
    || ( ( sig->pkt_version != 0 )
    && ( sig->pkt_version != COSM_PKI_VERSION ) )
    || ( sig->bits < 512 )
    || ( ( sig->type != COSM_PKI_SIG_MESSAGE )
    && ( sig->type != COSM_PKI_SIG_SIGN )
    && ( sig->type != COSM_PKI_SIG_KNOWN )
    && ( sig->type != COSM_PKI_SIG_WEAK )
    && ( sig->type != COSM_PKI_SIG_STRONG )
    && ( sig->type != COSM_PKI_SIG_TIMESTAMP )
    && ( sig->type != COSM_PKI_SIG_REVOKE ) )
    || ( ( sig->shared != COSM_PKI_SHARED_NO )
    && ( sig->shared != COSM_PKI_SHARED_YES ) ) )
  {
    return COSM_PKI_ERROR_FORMAT;
  }

  if ( max_bytes < (u64) ( ( sig->bits / 8 ) + 34 ) )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  /* now load the signature */
  if ( CosmBNLoad( &sig->sig, &buffer[34], sig->bits ) != COSM_PASS )
  {
    CosmBNFree( &sig->sig );
    return COSM_PKI_ERROR_MEMORY;
  }

  *bytes_read = (u64) ( ( sig->bits / 8 ) + 34 );

  return COSM_PASS;
}

s32 CosmPKISigSave( u8 * buffer, u64 * length, const cosm_PKI_SIG * sig,
  u64 max_bytes )
{
  if ( ( buffer == NULL ) || ( length == NULL ) || ( sig == NULL ) )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  if ( max_bytes < (u64) ( ( sig->bits / 8 ) + 34 ) )
  {
    return COSM_PKI_ERROR_FORMAT;
  }

  /* save number first for error checking */
  if ( CosmBNSave( &buffer[34], &sig->sig, sig->bits, sig->bits )
    != COSM_PASS )
  {
    return COSM_PKI_ERROR_MEMORY;
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

void CosmPKISigFree( cosm_PKI_SIG * sig )
{
  if ( sig == NULL )
  {
    return;
  }

  CosmBNFree( &sig->sig );

  CosmMemSet( sig, sizeof( cosm_PKI_SIG ), 0 );
}

s32 CosmPKIEncode( cosm_PKI_SIG * sig, const cosm_HASH * hash,
  cosmtime timestamp, u8 type, u8 shared,
  const cosm_PKI_KEY * key )
{
  cosm_BN bn_hash, tmp, tmp2;
  cosm_PRNG rnd;
  cosmtime now;
  u8 * padding;
  u32 pad;

  if ( ( sig == NULL ) || ( hash == NULL ) || ( key == NULL )
    || ( key->bits < 512 ) || ( ( shared != COSM_PKI_SHARED_NO )
    && ( shared != COSM_PKI_SHARED_YES ) ) )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  /* check key/type combinations */
  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    if ( type != COSM_PKI_SIG_MESSAGE )
    {
      return COSM_PKI_ERROR_PARAM;
    }
  }
  else if ( key->pkt_type == COSM_PKI_PRIVATE )
  {
    if ( ( type != COSM_PKI_SIG_SIGN ) && ( type != COSM_PKI_SIG_KNOWN )
      && ( type != COSM_PKI_SIG_WEAK ) && ( type != COSM_PKI_SIG_STRONG )
      && ( type != COSM_PKI_SIG_TIMESTAMP ) && ( type != COSM_PKI_SIG_REVOKE ) )
    {
      return COSM_PKI_ERROR_PARAM;
    }
  }
  else
  {
    return COSM_PKI_ERROR_PARAM;
  }

  /* is key currently valid? */
  CosmSystemClock( &now );
  if ( ( ( key->create > now.hi ) )
    || ( ( key->expire < now.hi ) ) )
  {
    return COSM_PKI_ERROR_EXPIRED;
  }

  /* is the timestamp in range */
  if ( ( ( key->create > timestamp.hi ) )
    || ( ( key->expire < timestamp.hi ) ) )
  {
    return COSM_PKI_ERROR_EXPIRED;
  }

  /* pad hash */
  pad = ( key->bits / 8 ) - 1 - 42;
  if ( ( padding = CosmMemAlloc( pad + 42LL ) )
    == NULL )
  {
    return COSM_PKI_ERROR_MEMORY;
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
    return COSM_PKI_ERROR_MEMORY;
  }

  if ( CosmBNLoad( &bn_hash, padding, key->bits - 8 ) != COSM_PASS )
  {
    CosmMemFree( padding );
    return COSM_PKI_ERROR_MEMORY;
  }

  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    /* public... sig = hash ^ COSM_BN_E mod key->n */
    if ( ( CosmBNSets32( &tmp, ( key->pkt_version > 0 ) ? COSM_BN_E : 17 )
      != COSM_PASS )
      || ( CosmBNModExp( &sig->sig, &bn_hash, &tmp, &key->n ) != COSM_PASS ) )
    {
      CosmBNFree( &bn_hash );
      CosmBNFree( &tmp );
      CosmMemFree( padding );
      return COSM_PKI_ERROR_MEMORY;
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
        return( COSM_PKI_ERROR_MEMORY );
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
        return COSM_PKI_ERROR_MEMORY;
      }

      if ( tmp.neg )
      {
        if ( CosmBNAdd( &tmp, &tmp, &key->p ) != COSM_PASS )
        {
          CosmBNFree( &bn_hash );
          CosmBNFree( &tmp );
          CosmBNFree( &tmp2 );
          CosmMemFree( padding );
          return COSM_PKI_ERROR_MEMORY;
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
        return COSM_PKI_ERROR_MEMORY;
      }
    }
  }

  CosmBNFree( &bn_hash );
  CosmBNFree( &tmp );
  CosmBNFree( &tmp2 );
  CosmMemFree( padding );

  sig->pkt_type = COSM_PKI_SIGNATURE;
  sig->pkt_version = COSM_PKI_VERSION;
  sig->bits = key->bits;
  sig->create = key->create;
  sig->id = key->id;
  sig->timestamp = timestamp.hi;

  sig->type = type;
  sig->shared = shared;

  return COSM_PASS;
}

s32 CosmPKIDecode( cosm_HASH * hash, cosmtime * timestamp, u8 * type,
  u8 * shared, const cosm_PKI_SIG * sig, const cosm_PKI_KEY * key )
{
  cosm_BN bn_hash, tmp, tmp2;
  u8 * padding;
  u32 pad;

  if ( ( hash == NULL ) || ( timestamp == NULL ) || ( type == NULL )
    || ( sig == NULL ) || ( key == NULL ) || ( key->bits < 512 ) )
  {
    return COSM_PKI_ERROR_PARAM;
  }

  /* check key/type combinations */
  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    if ( ( sig->type != COSM_PKI_SIG_SIGN )
      && ( sig->type != COSM_PKI_SIG_KNOWN )
      && ( sig->type != COSM_PKI_SIG_WEAK )
      && ( sig->type != COSM_PKI_SIG_STRONG )
      && ( sig->type != COSM_PKI_SIG_TIMESTAMP )
      && ( sig->type != COSM_PKI_SIG_REVOKE ) )
    {
      return COSM_PKI_ERROR_PARAM;
    }
  }
  else if ( key->pkt_type == COSM_PKI_PRIVATE )
  {
    if ( sig->type != COSM_PKI_SIG_MESSAGE )
    {
      return COSM_PKI_ERROR_PARAM;
    }
  }
  else
  {
    return COSM_PKI_ERROR_PARAM;
  }

  /* key is the ones used on the sig? */
  if ( ( key->id != sig->id )
    || ( key->create != sig->create )
    || ( key->bits != sig->bits ) )
  {
    return COSM_PKI_ERROR_FORMAT;
  }

  if ( ( CosmBNInit( &bn_hash ) != COSM_PASS ) || ( CosmBNInit( &tmp ) != COSM_PASS )
    || ( CosmBNInit( &tmp2 ) != COSM_PASS ) )
  {
    return COSM_PKI_ERROR_MEMORY;
  }

  if ( key->pkt_type == COSM_PKI_PUBLIC )
  {
    /* public, "hash" = sig->sig ^ COSM_BN_E mod key->n */
    if ( ( CosmBNSets32( &tmp, ( key->pkt_version > 0 ) ? COSM_BN_E : 17 )
      != COSM_PASS )
      || ( CosmBNModExp( &bn_hash, &sig->sig, &tmp, &key->n ) != COSM_PASS ) )
    {
      CosmBNFree( &bn_hash );
      CosmBNFree( &tmp );
      return COSM_PKI_ERROR_MEMORY;
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
        return( COSM_PKI_ERROR_MEMORY );
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
        return COSM_PKI_ERROR_MEMORY;
      }

      if ( tmp.neg )
      {
        if ( CosmBNAdd( &tmp, &tmp, &key->p ) != COSM_PASS )
        {
          CosmBNFree( &bn_hash );
          CosmBNFree( &tmp );
          CosmBNFree( &tmp2 );
          return COSM_PKI_ERROR_MEMORY;
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
        return COSM_PKI_ERROR_MEMORY;
      }
    }
  }

  /* space for our message */
  pad = ( key->bits / 8 ) - 1 - 42;
  if ( ( padding = CosmMemAlloc( (u64) ( key->bits / 8 ) ) ) == NULL )
  {
    CosmBNFree( &bn_hash );
    CosmBNFree( &tmp );
    return COSM_PKI_ERROR_MEMORY;
  }

  if ( CosmBNSave( padding, &bn_hash, key->bits - 8, key->bits - 8 )
    != COSM_PASS )
  {
    CosmBNFree( &bn_hash );
    CosmBNFree( &tmp );
    CosmMemFree( padding );
    return COSM_PKI_ERROR_MEMORY;
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
    || ( ( *type != COSM_PKI_SIG_MESSAGE )
    && ( *type != COSM_PKI_SIG_SIGN )
    && ( *type != COSM_PKI_SIG_KNOWN )
    && ( *type != COSM_PKI_SIG_WEAK )
    && ( *type != COSM_PKI_SIG_STRONG )
    && ( *type != COSM_PKI_SIG_TIMESTAMP )
    && ( *type != COSM_PKI_SIG_REVOKE ) )
    || ( ( *shared != COSM_PKI_SHARED_NO )
    && ( *shared != COSM_PKI_SHARED_YES ) ) )
  {
    return COSM_PKI_ERROR_FORMAT;
  }

  return COSM_PASS;
}

/* Rijndael API */

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

/* high level Rijndael */

typedef struct cosm_RIJNDAEL_CONTEXT
{
  u8 iv[16];
  u8 chain[16];
  u32 count;
  u32 key_bits;
  u8 key_schedule[MAXROUNDS+1][4][4];
  u32 mode;
  u32 direction;
} cosm_RIJNDAEL_CONTEXT;

s32 Cosm_RijndaelInit( cosm_TRANSFORM * transform, va_list params )
{
  cosm_RIJNDAEL_CONTEXT * context;
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
  if ( ( context = (cosm_RIJNDAEL_CONTEXT *) CosmMemAllocSecure(
    sizeof( cosm_RIJNDAEL_CONTEXT ) ) ) == NULL )
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

s32 Cosm_Rijndael( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_RIJNDAEL_CONTEXT * context;
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

s32 Cosm_RijndaelEnd( cosm_TRANSFORM * transform )
{
  cosm_RIJNDAEL_CONTEXT * context;
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
    Cosm_Rijndael( transform, padding, ( 16 - context->count ) );
  }

  /* output is now flushed */

  /* clear the context */
  CosmMemSet( context, sizeof( cosm_RIJNDAEL_CONTEXT ), 0 );
  CosmMemFree( context );

  return COSM_PASS;
}

/* cleanup defines */
#undef MAXKC
#undef MAXROUNDS

/* testing */

s32 Cosm_TestCrypto( void )
{
  cosm_BUFFER buffer;
  cosm_TRANSFORM transform;
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
  s32 error;

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

#endif
