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

/* Low level PKI functions, stubs or real code */

#if ( !defined( NO_CRYPTO ) )
#define COSM_CRYPT_C_INC
#include "crypto.c"
#undef COSM_CRYPT_C_INC
#else
s32 CosmPKIKeyGen( cosm_PKI_KEY * public_key, cosm_PKI_KEY * private_key,
  u32 bits, const u8 * rnd_bits, u64 id, const utf8 * alias,
  cosmtime create, cosmtime expire, void (*callback)( s32, s32, void * ),
  void * callback_param )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

s32 CosmPKIKeyLoad( cosm_PKI_KEY * key, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes, const cosm_HASH * pass_hash )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

s32 CosmPKIKeySave( u8 * buffer, u64 * length, const cosm_PKI_KEY * key,
  const u64 max_bytes, const u8 * rnd_bits,
  const cosm_HASH * pass_hash )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

void CosmPKIKeyFree( cosm_PKI_KEY * key )
{
  return;
}

s32 CosmPKISigLoad( cosm_PKI_SIG * sig, u64 * bytes_read,
  const u8 * buffer, u64 max_bytes )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

s32 CosmPKISigSave( u8 * buffer, u64 * length, const cosm_PKI_SIG * sig,
  u64 max_bytes )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

void CosmPKISigFree( cosm_PKI_SIG * sig )
{
  return;
}

s32 CosmPKIEncode( cosm_PKI_SIG * sig, const cosm_HASH * hash,
  cosmtime timestamp, u8 type, u8 shared, const cosm_PKI_KEY * key )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

s32 CosmPKIDecode( cosm_HASH * hash, cosmtime * timestamp, u8 * type,
  u8 * shared, const cosm_PKI_SIG * sig, const cosm_PKI_KEY * key )
{
  return COSM_PKI_ERROR_NO_CRYPTO;
}

#endif /* NO_CRYPTO */

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

  for( i = 0; i < 16; i++ )
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

  for( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 1 );

  for( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 2 );

  for( i = 0; i < 20; i++ )
    _COSM_SHA_FG( 3 );

  for( i = 0; i < 20; i++ )
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

  for( i = 0; i < 16; i++ )
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

  /* possible Crypto tests, errors start at -1000 */
#if ( !defined( NO_CRYPTO ) )
  return Cosm_TestCrypto();
#else
  return COSM_PASS;
#endif

test_failed:
  CosmTransformEnd( &transform );
  return error;
}
