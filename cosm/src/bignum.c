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

#include "cosm/bignum.h"
#include "cosm/os_math.h"
#include "cosm/os_io.h"
#include "cosm/buffer.h"
#include "cosm/security.h"

s32 CosmBNInit( cosm_BN * x )
{
  if ( x == NULL )
  {
    return COSM_FAIL;
  }

  return CosmMemSet( x, sizeof( cosm_BN ), 0 );
}

s32 CosmBNSet( cosm_BN * x, const cosm_BN * a )
{
  COSM_BN_SWORD i;

  if ( ( x == NULL ) || ( a == NULL )
    || ( Cosm_BNGrow( x, a->digits ) != COSM_PASS ) )
  {
    return COSM_FAIL;
  }

  x->digits = a->digits;
  x->neg = a->neg;

  for ( i = 0 ; i < a->digits ; i++ )
  {
    x->n[i] = a->n[i];
  }
  for ( ; i < (COSM_BN_SWORD) x->length ; i++ )
  {
    x->n[i] = 0;
  }

  return COSM_PASS;
}

s32 CosmBNSets32( cosm_BN * x, const s32 a )
{
  COSM_BN_SWORD i;

  if ( ( x == NULL ) || ( Cosm_BNGrow( x, 1 ) != COSM_PASS ) )
  {
    return COSM_FAIL;
  }

  x->neg = ( a < 0 ) ? 1 : 0;
  x->n[0] = ( a < 0 ) ? -a : a;

  for ( i = ( x->digits - 1 ) ; i > 1 ; i-- )
  {
    x->n[i] = 0;
  }
  x->digits = 1;

  return COSM_PASS;
}

s32 CosmBNGets32( s32 * x, const cosm_BN * a )
{
  if ( ( x == NULL ) || ( a == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( a->digits == 0 )
  {
    *x = 0;
  }
  else
  {
    *x = (u32) a->n[0] & (u32) 0x7FFFFFFF;
    if ( a->neg )
    {
      *x = -*x;
    }
  }

  return COSM_PASS;
}

s32 CosmBNCmp( const cosm_BN * a, const cosm_BN * b )
{
  s32 result;

  if ( ( a == NULL ) || ( b == NULL ) )
  {
    return 0;
  }

  if ( a->neg ^ b->neg )
  {
    if ( a->neg )
    {
      return -1;
    }
    else
    {
      return 1;
    }
  }

  result = Cosm_BNuCmp( a, b );

  /* both negative */
  if ( a->neg )
  {
    return -result;
  }

  /* both positive */
  return result;
}

s32 CosmBNOdd( const cosm_BN * x )
{
  return ( x != NULL ) && ( x->digits > 0 ) && ( x->n[0] & 0x01 );
}

s32 CosmBNZero( const cosm_BN * x )
{
  return ( x != NULL ) && ( x->digits == 0 );
}

s32 CosmBNBits( u32 * x, const cosm_BN * a )
{
  COSM_BN_WORD mask;
  COSM_BN_WORD bits;
  COSM_BN_WORD word;

  if ( ( x == NULL ) || ( a == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( CosmBNZero( a ) )
  {
    *x = 0;
    return COSM_PASS;
  }

  bits = ( a->digits - 1 ) * COSM_BN_BITS;

  mask = 1;
  word = a->n[a->digits - 1];

  do
  {
    bits++;
    mask = mask << 1;
  } while ( ( word >= mask ) && ( mask != 0 ) );

  *x = (u32) bits;

  return COSM_PASS;
}

s32 CosmBNAdd( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  const cosm_BN * swap;
  u32 neg;

  /*
     a +  b = + ( a + b )
     a + -b = ? ( a - b )
    -a +  b = ? ( b - a )
    -a + -b = - ( a + b )
  */

  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( a->neg ^ b->neg )
  {
    /* make a positive and b negative, "a - b" */
    if ( a->neg )
    {
      swap = a;
      a = b;
      b = swap;
    }

    if ( Cosm_BNuCmp( a, b ) < 0 )
    {
      /* b > a, so result is negative */
      neg = 1;
      if ( Cosm_BNuSub( x, b, a ) != COSM_PASS )
      {
        return COSM_FAIL;
      }
    }
    else
    {
      /* a > b, result is positive */
      neg = 0;
      if ( Cosm_BNuSub( x, a, b ) != COSM_PASS )
      {
        return COSM_FAIL;
      }
    }
    x->neg = neg;
    return COSM_PASS;
  }

  if ( a->neg )
  {
    neg = 1;
  }
  else
  {
    neg = 0;
  }

  if ( Cosm_BNuAdd( x, a, b ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  x->neg = neg;
  return COSM_PASS;
}

s32 CosmBNSub( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  const cosm_BN * swap;
  u32 neg;

  /*
     a -  b = ? ( a - b )
     a - -b = + ( a + b )
    -a -  b = - ( a + b )
    -a - -b = ? ( b - a )
  */

  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( a->neg )
  {
    if ( b->neg )
    {
      /* ? ( b - a ) */
      swap = a;
      a = b;
      b = swap;
    }
    else
    {
      /* - ( a + b ) */
      neg = 1;
      if ( Cosm_BNuAdd( x, a, b ) != COSM_PASS )
      {
        return COSM_FAIL;
      }
      x->neg = neg;
      return COSM_PASS;
    }
  }
  else
  {
    if ( b->neg )
    {
      /* + ( a + b ) */
      neg = 0;
      if ( Cosm_BNuAdd( x, a, b ) != COSM_PASS )
      {
        return COSM_FAIL;
      }
      x->neg = neg;
      return COSM_PASS;
    }
  }

  /* ? ( a - b ) */
  if ( Cosm_BNuCmp( a, b ) < 0 )
  {
    /* b > a, so result is negative */
    neg = 1;
    if ( Cosm_BNuSub( x, b, a ) != COSM_PASS )
    {
      return COSM_FAIL;
    }
  }
  else
  {
    /* a > b, result is positive */
    neg = 0;
    if ( Cosm_BNuSub( x, a, b ) != COSM_PASS )
    {
      return COSM_FAIL;
    }
  }

  x->neg = neg;

  return COSM_PASS;
}

s32 CosmBNMul( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  u32 neg;

  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL ) )
  {
    return COSM_FAIL;
  }

  neg = ( a->neg ^ b->neg );

  if ( Cosm_BNuMul( x, a, b ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  x->neg = neg;

  return COSM_PASS;
}

s32 CosmBNDiv( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL ) )
  {
    return COSM_FAIL;
  }

  return Cosm_BNDivMod( x, NULL, a, b );
}

s32 CosmBNMod( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL ) )
  {
    return COSM_FAIL;
  }

  return Cosm_BNDivMod( NULL, x, a, b );
}

s32 CosmBNModExp( cosm_BN * x, const cosm_BN * a, const cosm_BN * e,
  const cosm_BN * m )
{
  cosm_BN tmp_x, a_n;
  u32 i, bits;

  if ( ( x == NULL ) || ( a == NULL ) || ( e == NULL ) || ( m == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( ( a->digits == 0 ) || ( m->digits == 0 )
    || ( ( m->digits == 1 ) && ( m->n[0] == 1 ) ) )
  {
    /* a = 0, m = 0, or m = 1 */
    CosmBNSets32( x, 0 );
    return COSM_PASS;
  }

  if ( e->digits == 0 )
  {
    /* e = 0 */
    CosmBNSets32( x, 1 );
    return COSM_PASS;
  }

  if ( CosmBNInit( &tmp_x ) || CosmBNInit( &a_n ) )
  {
    return COSM_FAIL;
  }

  CosmBNSets32( &tmp_x, 1 );
  CosmBNSet( &a_n, a );

  /*
    tmp_x = 1, a_n = a, now we just step through the bits of e and
    multiply & mod each time we find a bit of e = 1.
  */
  CosmBNBits( &bits, e );
  for ( i = 0 ; i < bits ; i++ )
  {
    /* if e(bit) is 1, multiply and mod tmp_x */
    if ( e->n[i / COSM_BN_BITS]
      & ( ( (COSM_BN_WORD) 1 ) << ( i % COSM_BN_BITS ) ) )
    {
      CosmBNMul( &tmp_x, &tmp_x, &a_n );
      Cosm_BNDivMod( NULL, &tmp_x, &tmp_x, m );
    }

    /* update a_n */
    CosmBNMul( &a_n, &a_n, &a_n );
    Cosm_BNDivMod( NULL, &a_n, &a_n, m );
  }

  CosmBNSet( x, &tmp_x );

  CosmBNFree( &tmp_x );
  CosmBNFree( &a_n );

  return COSM_PASS;
}

s32 CosmBNModInv( cosm_BN * x, const cosm_BN * a, const cosm_BN * m )
{
  cosm_BN big, little, q, r, t0, t, temp;
  s32 result;

  if ( ( x == NULL ) || ( a == NULL ) || ( m == NULL )
   || CosmBNZero( a ) || CosmBNZero( m ) || ( -1 != CosmBNCmp( a, m ) ) )
  {
    return COSM_FAIL;
  }

  if ( CosmBNInit( &big ) || CosmBNInit( &little ) || CosmBNInit( &q )
    || CosmBNInit( &r ) || CosmBNInit( &t0 ) || CosmBNInit( &t )
    || CosmBNInit( &temp ) )
  {
    result = COSM_FAIL;
    goto modinv_failed;
  }

  CosmBNSet( &big, m );
  CosmBNSet( &little, a );
  CosmBNSets32( &t0, 0 );
  CosmBNSets32( &t, 1 );

  /* q = big div little, r = big mod little */
  Cosm_BNDivMod( &q, &r, &big, &little );

  while ( !CosmBNZero( &r ) )
  {
    /* temp = t0 - q * t */
    CosmBNMul( &temp, &q, &t );
    CosmBNSub( &temp, &t0, &temp );

    /* temp = temp mod m */
    Cosm_BNDivMod( NULL, &temp, &temp, m );

    /* t0 = t, t = temp */
    CosmBNSet( &t0, &t );
    CosmBNSet( &t, &temp );

    /* big = little, little = r */
    CosmBNSet( &big, &little );
    CosmBNSet( &little, &r );

    /* q = big div little, r = big mod little */
    Cosm_BNDivMod( &q, &r, &big, &little );
  }

  /* if little != 1, we have no inverse */
  CosmBNSets32( &temp, 1 );
  if ( 0 != CosmBNCmp( &temp, &little ) )
  {
    result = COSM_PASS;
    goto modinv_failed;
  }

  /* we found the inverse and it's t */
  CosmBNSet( x, &t );
  result = COSM_PASS;

  /* cleanup and return */
modinv_failed:
  CosmBNFree( &big );
  CosmBNFree( &little );
  CosmBNFree( &q );
  CosmBNFree( &r );
  CosmBNFree( &t0 );
  CosmBNFree( &t );
  CosmBNFree( &temp );

  return result;
}

s32 CosmBNGCD( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  cosm_BN tmp, ta, tb;

  if ( ( x == NULL ) || ( a == NULL ) || ( b == NULL )
   || CosmBNZero( a ) || CosmBNZero( b ) )
  {
    return COSM_FAIL;
  }

  if ( CosmBNInit( &tmp ) || CosmBNInit( &ta ) || CosmBNInit( &tb ) )
  {
    return COSM_FAIL;
  }

  /* ta = the greater number, tb = lesser */
  if ( CosmBNCmp( a, b ) == -1 )
  {
    if ( CosmBNSet( &ta, b ) || CosmBNSet( &tb, a ) )
    {
      CosmBNFree( &tmp );
      CosmBNFree( &ta );
      CosmBNFree( &tb );
      return COSM_FAIL;
    }
  }
  else
  {
    if ( CosmBNSet( &ta, a ) || CosmBNSet( &tb, b ) )
    {
      CosmBNFree( &tmp );
      CosmBNFree( &ta );
      CosmBNFree( &tb );
      return COSM_FAIL;
    }
  }

  ta.neg = 0;
  tb.neg = 0;

  /* Euclid */
  while ( ta.digits > 0 )
  {
    if ( CosmBNSet( &tmp, &ta )
      || Cosm_BNDivMod( NULL, &ta, &tb, &ta )
      || CosmBNSet( &tb, &tmp ) )
    {
      CosmBNFree( &tmp );
      CosmBNFree( &ta );
      CosmBNFree( &tb );
      return COSM_FAIL;
    }
  }

  CosmBNFree( &ta );
  CosmBNFree( &tb );

  if ( CosmBNSet( x, &tmp ) )
  {
    CosmBNFree( &tmp );
    return COSM_FAIL;
  }

  CosmBNFree( &tmp );

  return COSM_PASS;
}

s32 CosmBNPrimeRM( const cosm_BN * p, u32 tests,
  void (*callback)( s32, s32, void * ), void * param )
{
  cosm_BN m, a, z, p1, two;
  u32 b, i, j;
  u32 bits;
  cosm_PRNG rnd;
  u8 * rnd_bits;
  s32 result;

  if ( ( p == NULL ) || ( !CosmBNOdd( p ) ) )
  {
    return 0;
  }

  CosmBNBits( &bits, p );

  if ( tests < 5 )
  {
    tests = ( bits > 511 ) ?  5 : ( bits >= 450 ) ?  6 :
      ( bits >= 400 ) ?  7 : ( bits >= 350 ) ?  8 : ( bits >= 300 ) ?  9 :
      ( bits >= 250 ) ? 12 : ( bits >= 200 ) ? 15 : ( bits >= 150 ) ? 18 : 27;
  }

  CosmBNInit( &m );
  CosmBNInit( &a );
  CosmBNInit( &z );
  CosmBNInit( &p1 );
  CosmBNInit( &two );

  /* x = 1 + 2^b * m, p1 = p - 1, two = 2 */
  CosmBNSet( &m, p );
  Cosm_BNBitSet( &m, 0, 0 );
  CosmBNSet( &p1, &m );
  b = 0;
  while ( !CosmBNOdd( &m ) )
  {
    Cosm_BNRsh1( &m );
    b++;
  }
  CosmBNSets32( &two, 2 );

  /* see random generator with x */
  CosmMemSet( &rnd, sizeof( cosm_PRNG ), 0 );
  CosmPRNG( &rnd, NULL, (u64) 0,
    (u8 *) p->n, (u64) p->digits * COSM_BN_BYTES );

  /* for our bits */
  rnd_bits = CosmMemAlloc( (u64) bits / 8 );

  for ( i = 0 ; i < tests ; i++ )
  {
    if ( callback != NULL )
    {
      (*callback)( 1, i, param );
    }
    /* a = random number one less bit then p */
    CosmPRNG( &rnd, rnd_bits, (u64) bits / 8, NULL, 0 );
    rnd_bits[0] = (u8) ( ( rnd_bits[0] & 0x7F ) | 0x40 );
    CosmBNLoad( &a, rnd_bits, bits );

    /* z = a^m mod p */
    CosmBNModExp( &z, &a, &m, p );

    /* passing condition A) z = 1 here */

    if ( ( z.digits == 1 ) && ( z.n[0] == 1 ) )
    {
      continue;
    }

    /* passing condition B) can find z = p-1 within b-1 squarings */

    for ( j = 0 ; j < b ; j++ )
    {
      /* skip squaring first time, z already set */
      if ( j > 0 )
      {
        /* z = z^2 mod p */
        CosmBNModExp( &z, &z, &two, p );
      }
      if ( CosmBNCmp( &z, &p1 ) == 0 )
      {
        /* pass, break this inner for loop */
        break;
      }
    }
    /* fail if we never found z == p1 */
    if ( j == b )
    {
      break;
    }
  }

  /* passed all tests? */
  if ( i == tests )
  {
    result = 1;
  }
  else
  {
    result = 0;
  }

  CosmBNFree( &m );
  CosmBNFree( &a );
  CosmBNFree( &z );
  CosmBNFree( &p1 );
  CosmBNFree( &two );
  CosmMemFree( rnd_bits );

  return result;
}

#define PRIME_COUNT 303
static s32 small_primes[PRIME_COUNT] =
{
  /*
    Dont need 2, since we know about odd/even already,
    But we DO need to test that the prime is relatively prime
    to our choice of e, so small_primes[0] == COSM_BN_E.
    We do this here becasue it is a trivial check here, but
    very costly later.
  */
  COSM_BN_E, 3,    5,    7,   11,   13,   17,   19,
    23,   29,   31,   37,   41,   43,   47,   53,
    59,   61,   67,   71,   73,   79,   83,   89,
    97,  101,  103,  107,  109,  113,  127,  131,
   137,  139,  149,  151,  157,  163,  167,  173,
   179,  181,  191,  193,  197,  199,  211,  223,
   227,  229,  233,  239,  241,  251,  257,  263,
   269,  271,  277,  281,  283,  293,  307,  311,
   313,  317,  331,  337,  347,  349,  353,  359,
   367,  373,  379,  383,  389,  397,  401,  409,
   419,  421,  431,  433,  439,  443,  449,  457,
   461,  463,  467,  479,  487,  491,  499,  503,
   509,  521,  523,  541,  547,  557,  563,  569,
   571,  577,  587,  593,  599,  601,  607,  613,
   617,  619,  631,  641,  643,  647,  653,  659,
   661,  673,  677,  683,  691,  701,  709,  719,
   727,  733,  739,  743,  751,  757,  761,  769,
   773,  787,  797,  809,  811,  821,  823,  827,
   829,  839,  853,  857,  859,  863,  877,  881,
   883,  887,  907,  911,  919,  929,  937,  941,
   947,  953,  967,  971,  977,  983,  991,  997,
  1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049,
  1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097,
  1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163,
  1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
  1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283,
  1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321,
  1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423,
  1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459,
  1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
  1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571,
  1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619,
  1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693,
  1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747,
  1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
  1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877,
  1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949,
  1951, 1973, 1979, 1987, 1993, 1997, 1999
};

s32 CosmBNPrimeGen( cosm_BN * x, u32 bits, const u8 * rnd_bits,
  void (*callback)( s32, s32, void * ), void * param )
{
  cosm_BN p, tmp;
  s32 mods[PRIME_COUNT];
  s32 base, add;
  u32 i;

  if ( ( x == NULL ) || ( bits < 256 ) || ( ( bits % 8 ) != 0 )
    || ( rnd_bits == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( CosmBNInit( &p ) || CosmBNInit( &tmp ) )
  {
    return COSM_FAIL;
  }

  /* load bits as a possible prime p */
  CosmBNLoad( &p, rnd_bits, bits );

  /* set top and bottom bits */
  Cosm_BNBitSet( &p, bits - 1, 1 );
  Cosm_BNBitSet( &p, 0, 1 );

  /* setup "base" mods */
  for ( i = 0 ; i < PRIME_COUNT ; i++ )
  {
    CosmBNSets32( &tmp, small_primes[i] );
    Cosm_BNDivMod( NULL, &tmp, &p, &tmp );
    CosmBNGets32( &mods[i], &tmp );
  }

  /* modify mods[0] for special e test for (p-1) % small_primes[0] == 0 */
  mods[0] = ( mods[0] + small_primes[0] - 1 ) % small_primes[0];

  add = 0;
  base = 0;
  for ( ; ; )
  {
    /* add 2 until we pass all the small prime tests */
    for ( ; ; )
    {
      if ( callback != NULL )
      {
        (*callback)( 0, add / 2, param );
      }
      /* repeat until all the mods are non-zero */
      i = 0;
      while ( ( i < PRIME_COUNT )
        && ( ( ( mods[i] + add ) % small_primes[i] ) != 0 ) )
      {
        i++;
      }

      /* if we checked all the mods successfully we are done */
      if ( i == PRIME_COUNT )
      {
        break;
      }

      if ( add > 0x7FFF0000 )
      {
        /* like this will ever happen */
        CosmBNFree( &p );
        CosmBNFree( &tmp );
        return COSM_FAIL;
      }

      /* try again */
      add += 2;
    }

    CosmBNSets32( &tmp, ( add - base ) );
    CosmBNAdd( &p, &p, &tmp );
    base = add;
    add += 2;

    /* check that p isn't more bits now */
    if ( ( CosmBNBits( &i, &p ) != COSM_PASS ) || ( i != bits ) )
    {
      CosmBNFree( &p );
      CosmBNFree( &tmp );
      return COSM_FAIL;
    }

    /* run MR test */
    if ( !CosmBNPrimeRM( &p, 0, callback, param ) )
    {
      /* failed MR */
      continue;
    }

    /* found one */
    CosmBNSet( x, &p );
    CosmBNFree( &p );
    CosmBNFree( &tmp );

    if ( callback != NULL )
    {
      (*callback)( 2, 0, param );
    }

    return COSM_PASS;
  }

  return COSM_FAIL;
}

s32 CosmBNStr( cosm_BN * x, void ** end, const void * string, u32 radix )
{
  return COSM_FAIL;
}

static const ascii hex_table[16] =
{
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/* Macro for making sure we don't print too much */
#define _COSM_BN_CHECKMAX() { \
  chars_output++; \
  if ( string != NULL ) \
  { \
    string++; \
  } \
  if ( ( chars_output == max_bytes ) ) \
  { \
    if ( string != NULL ) \
    { \
      *string = 0; \
    } \
    CosmBNFree( &tmp ); \
    CosmBNFree( &rad ); \
    CosmBNFree( &digit ); \
    CosmBufferFree( &buf ); \
    return chars_output; \
  } }

u32 CosmBNPrint( cosm_FILE * file, utf8 * string, u32 max_bytes,
  u32 radix, cosm_BN * x )
{
  cosm_BUFFER buf;
  cosm_BN tmp, rad, digit;
  u32 chars_output;
  u32 bytes;
  s32 ch;
  u64 one;
  ascii asc;

  chars_output = 0;

  if ( ( max_bytes > 0 ) )
  {
    max_bytes--;
  }

  if ( ( max_bytes == chars_output ) || ( radix < 2 )
    || ( radix > 16 ) || ( x == NULL ) )
  {
    return chars_output;
  }

  /* setup buffer */
  CosmMemSet( &buf, sizeof( cosm_BUFFER ), 0 );
  if ( ( CosmBufferInit( &buf, (u64) 4096, COSM_BUFFER_MODE_STACK,
    (u64) 4096, NULL, 0 ) != COSM_PASS ) )
  {
    return chars_output;
  }

  if ( CosmBNInit( &tmp ) || CosmBNInit( &rad ) || CosmBNInit( &digit ) )
  {
    CosmBufferFree( &buf );
    return chars_output;
  }

  if ( CosmBNSet( &tmp, x ) )
  {
    CosmBufferFree( &buf );
    return chars_output;
  }

  /* is the number 0 ? */
  if ( ( tmp.n == NULL ) || ( tmp.digits == 0 ) )
  {
    if ( Cosm_PrintChar( &bytes, file, string, 1, '0' ) != COSM_PASS )
    {
      CosmBNFree( &tmp );
      CosmBufferFree( &buf );
      return chars_output;
    }
    _COSM_BN_CHECKMAX();
    if ( string != NULL )
    {
      *string = 0;
    }
    CosmBNFree( &tmp );
    CosmBufferFree( &buf );
    return chars_output;
  }

  /* negative ? */
  if ( tmp.neg == 1 )
  {
    if ( Cosm_PrintChar( &bytes, file, string, 1, '-' ) != COSM_PASS )
    {
      CosmBNFree( &tmp );
      CosmBufferFree( &buf );
      return chars_output;
    }
    _COSM_BN_CHECKMAX();
    tmp.neg = 0;
  }

  /* fill buffer */
  CosmBNSets32( &rad, radix );
  one = 1LL;
  do
  {
    Cosm_BNDivMod( &tmp, &digit, &tmp, &rad );
    CosmBNGets32( &ch, &digit );
    asc = hex_table[ch];
    if ( CosmBufferPut( &buf, &asc, one ) != COSM_PASS )
    {
      CosmBNFree( &tmp );
      CosmBNFree( &rad );
      CosmBNFree( &digit );
      CosmBufferFree( &buf );
      return chars_output;
    }
  } while ( tmp.digits > 0 );

  CosmBNFree( &tmp );
  CosmBNFree( &rad );
  CosmBNFree( &digit );

  /* reverse print as many chars as we can */
  while ( CosmBufferGet( &asc, one, &buf ) == one )
  {
    if ( Cosm_PrintChar( &bytes, file, string, 1, asc ) != COSM_PASS )
    {
      CosmBufferFree( &buf );
      return chars_output;
    }
    _COSM_BN_CHECKMAX();
  }

  CosmBufferFree( &buf );

  if ( string != NULL )
  {
    /* terminate the string */
    *string = 0;
  }

  return chars_output;
}

void CosmBNFree( cosm_BN * x )
{
  if ( x == NULL )
  {
    return;
  }

  CosmMemFree( x->n );
  x->n = NULL;
  x->digits = 0;
  x->length = 0;
  x->neg = 0;
}

s32 CosmBNLoad( cosm_BN * x, const u8 * bytes, u32 bits )
{
  cosm_BN tmp;
  COSM_BN_WORD word;
  u32 len;
  s32 i;

  if ( ( x == NULL ) || ( bytes == NULL ) || ( ( bits % 8 ) != 0 )
    || ( bits > COSM_BN_MAXBITS ) )
  {
    return COSM_FAIL;
  }

  if ( CosmBNInit( &tmp ) )
  {
    return COSM_FAIL;
  }

  /* load the "extra" bytes */
  if ( ( len = bits % COSM_BN_BITS ) != 0 )
  {
    if ( Cosm_BNGrow( &tmp, 1 ) != COSM_PASS )
    {
      CosmBNFree( &tmp );
      return COSM_FAIL;
    }
    tmp.digits = 1;
    for ( i = 0 ; i < (s32) ( ( len + 7 ) / 8 ) ; i++ )
    {
      tmp.n[0] = ( tmp.n[0] << 8 ) + *(bytes++);
    }
  }

  /* load remaining data */
  len = bits / COSM_BN_BITS;
  if ( len > 0 )
  {
    /* left shift */
    if ( tmp.digits > 0 )
    {
      if ( Cosm_BNLsh( &tmp, &tmp, len * COSM_BN_BITS ) != COSM_PASS )
      {
        CosmBNFree( &tmp );
        return COSM_FAIL;
      }
    }
    else
    {
      Cosm_BNGrow( &tmp, len );
      tmp.digits = len;
    }

    /* fill in the digits with CosmLoad */
    for ( i = len - 1 ; i >= 0 ; i-- )
    {
#if ( COSM_BN_BITS == 64 )
      CosmU64Load( &word, bytes );
#else
      CosmU32Load( &word, bytes );
#endif
      tmp.n[i] = word;
      bytes = &bytes[COSM_BN_BYTES];
    }
  }

  /* make sure we dont have extra 0's */
  while ( ( tmp.digits > 0 ) && ( tmp.n[tmp.digits - 1] == 0 ) )
  {
    tmp.digits--;
  }

  if ( CosmBNSet( x, &tmp ) != COSM_PASS )
  {
    return COSM_FAIL;
  }
  CosmBNFree( &tmp );

  return COSM_PASS;
}

s32 CosmBNSave( u8 * bytes, const cosm_BN * x, u32 max_bits, u32 min_bits )
{
  COSM_BN_WORD word;
  u32 my_bits, len;
  s32 i;

  if ( ( bytes == NULL ) || ( x == NULL ) || ( ( max_bits % 8 ) != 0 )
   || ( ( min_bits % 8 ) != 0 ) )
  {
    return COSM_FAIL;
  }

  CosmBNBits( &my_bits, x );

  /* check we arent too long */
  if ( ( my_bits > COSM_BN_MAXBITS ) || ( my_bits > max_bits ) )
  {
    return COSM_FAIL;
  }

  /* pad if we are too short */
  while ( ( min_bits / 8 ) > ( ( my_bits + 7 ) / 8 ) )
  {
    *bytes++ = 0;
    min_bits -= 8;
  }

  /* save the "extra" bytes */
  if ( ( len = my_bits % COSM_BN_BITS ) != 0 )
  {
    word = x->n[x->digits - 1];
    for ( i = (s32) ( ( len + 7 ) / 8 ) - 1 ; i >= 0; i-- )
    {
      *(bytes++) = (u8) ( word >> ( i * 8 ) );
    }
  }

  /* save remaining data */
  len = my_bits / COSM_BN_BITS;
  if ( len > 0 )
  {
    /* fill in the digits with CosmSave */
    for ( i = len - 1 ; i >= 0 ; i-- )
    {
      word = x->n[i];
#if ( COSM_BN_BITS == 64 )
      CosmU64Save( bytes, &word );
#else
      CosmU32Save( bytes, &word );
#endif
      bytes = &bytes[COSM_BN_BYTES];
    }
  }

  return COSM_PASS;
}

/* low level */

s32 Cosm_BNGrow( cosm_BN * x, const COSM_BN_WORD length )
{
  void * mem;
  COSM_BN_WORD i;

  if ( x->length >= length )
  {
    return COSM_PASS;
  }

  /* limit us to 2^32-64 */
  if ( length > COSM_BN_WLIMIT )
  {
    return COSM_PASS;
  }

  if ( ( mem = CosmMemRealloc( x->n, length * COSM_BN_BYTES ) ) == NULL )
  {
    return COSM_FAIL;
  }

  x->n = mem;
  x->length = length;

  /* must zero the Realloc'd space manually */
  for ( i = x->digits ; i < length ; i ++ )
  {
    x->n[i] = 0;
  }

  return COSM_PASS;
}

s32 Cosm_BNExplode( cosm_BN * x )
{
  COSM_BN_SWORD i;
  COSM_BN_WORD t;

  if ( Cosm_BNGrow( x, x->digits * 2 ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  x->digits *= 2;

  i = x->digits - 1;
  while ( i >= 0 )
  {
    t = x->n[i/2];
    x->n[i] = ( t >> ( COSM_BN_BITS / 2 ) );
    i--;
    x->n[i] = ( t & COSM_BN_MASK );
    i--;
  }

  if ( ( x->digits > 0 ) && ( x->n[x->digits - 1] == 0 ) )
  {
    x->digits--;
  }

  return COSM_PASS;
}

void Cosm_BNImplode( cosm_BN * x )
{
  COSM_BN_SWORD i;
  void * mem;

  x->digits = ( x->digits + 1 ) / 2;

  for ( i = 0 ; i < x->digits ; i++ )
  {
    x->n[i] = ( x->n[( i * 2 ) + 1] << ( COSM_BN_BITS / 2 ) ) + x->n[i * 2];
  }

  /* recover memory */

  if ( ( mem = CosmMemRealloc( x->n, x->digits * COSM_BN_BYTES ) ) == NULL )
  {
    /* non-fatal */
    return;
  }
  x->n = mem;
  x->length = x->digits;
}

s32 Cosm_BNuAdd( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  const cosm_BN * swap;
  COSM_BN_WORD * pa, * pb, * px;
  COSM_BN_SWORD i;
#if ( defined( COSM_BN_DWORD ) )
  COSM_BN_DWORD carry;
#else
  COSM_BN_WORD t, y, carry;
#endif

  /* dont touch x->neg */

  if ( a->digits < b->digits )
  {
    swap = a;
    a = b;
    b = swap;
  }

  if ( Cosm_BNGrow( x, a->digits + 1 ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  x->digits = a->digits;

  carry = 0;
  pa = a->n;
  pb = b->n;
  px = x->n;

#if ( defined( COSM_BN_DWORD ) )
  /* while we have some b */
  for ( i = 0 ; i < b->digits ; i++ )
  {
    carry += (COSM_BN_DWORD) *(pa++) + (COSM_BN_DWORD) *(pb++);
    *(px++) = (COSM_BN_WORD) carry;
    carry >>= COSM_BN_BITS;
  }

  /* then until we dont carry anymore */
  while ( carry )
  {
    carry += (COSM_BN_DWORD) *(pa++);
    *(px++) = (COSM_BN_WORD) carry;
    carry >>= COSM_BN_BITS;
    i++;
  }
#else
  /* while we have some b */
  for ( i = 0 ; i < b->digits ; i++ )
  {
    t = *(pa++) + carry;
    carry = ( x == 0 );
    y = t + *(pb++);
    carry += ( y < t );
    *(px++) = y;
  }

  /* then until we dont carry anymore */
  while ( carry )
  {
    t = *(pa++) + carry;
    carry = ( t == 0 );
    *(px++) = t;
    i++;
  }
#endif

  /* if we carried past the end of a, we're done */
  if ( i > a->digits )
  {
    *(px++) = 1;
    x->digits++;
    return COSM_PASS;
  }

  /* then copy the rest */
  while ( i < a->digits )
  {
    *(px++) = *(pa++);
    i++;
  }

  return COSM_PASS;
}

s32 Cosm_BNuSub( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  COSM_BN_WORD t, carry, * pa, * pb, * px;
  COSM_BN_SWORD i;

  /* dont touch x->neg */

  /* we already tested that a >= b */
  if ( Cosm_BNGrow( x, a->digits ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  carry = 0;
  pa = a->n;
  pb = b->n;
  px = x->n;

  /* while we have some b */
  for ( i = 0 ; i < b->digits ; i++ )
  {
    if ( carry )
    {
      carry = ( *pa <= *pb );
      t = *(pa++) - *(pb++) - 1;
    }
    else
    {
      carry = ( *pa < *pb );
      t = *(pa++) - *(pb++);
    }
    *(px++) = t;
  }

  /* then until we dont carry anymore */
  while ( carry )
  {
    carry = ( *pa == 0 );
    *(px++) = *(pa++) - 1;
    i++;
  }

  /* then copy the rest */
  while ( i < a->digits )
  {
    *(px++) = *(pa++);
    i++;
  }

  /* set correct x->digits, x may be shorter then a was */
  /* i is already a->digits here */
  while ( ( *(--px) == 0 ) && ( i > 0 ) )
  {
    i--;
  }
  x->digits = i;

  return COSM_PASS;
}

s32 Cosm_BNFastSub( cosm_BN * x, const cosm_BN * a )
{
  COSM_BN_WORD carry, * px, * pa;
  COSM_BN_SWORD i;

  carry = 0;
  px = x->n;
  pa = a->n;

  /* while we have some a substract and carry */
  for ( i = 0 ; i < a->digits ; i++ )
  {
    if ( carry )
    {
      carry = ( *px <= *pa );
      *px = *px - *pa - 1;
    }
    else
    {
      carry = ( *px < *pa );
      *px -= *pa;
    }
    px++;
    pa++;
  }

  /* then until we dont carry anymore */
  while ( carry )
  {
    carry = ( *px == 0 );
    *(px++) -= 1;
    i++;
  }

  /* set correct x->digits */
  while ( ( x->n[x->digits - 1] == 0 ) && ( x->digits > 0 ) )
  {
    x->digits--;
  }

  return COSM_PASS;
}

s32 Cosm_BNuMul( cosm_BN * x, const cosm_BN * a, const cosm_BN * b )
{
  cosm_BN tmp;
  COSM_BN_WORD * pa, * px;
  COSM_BN_WORD m;
  COSM_BN_SWORD i, j;
  s32 ret;
#if ( defined( COSM_BN_DWORD ) )
  const cosm_BN * swap;
  COSM_BN_DWORD carry;
#else
  COSM_BN_WORD carry;
  cosm_BN ta, tb;
#endif

  if ( CosmBNInit( &tmp ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

#if ( defined( COSM_BN_DWORD ) )
  if ( Cosm_BNGrow( &tmp, a->digits + b->digits ) != COSM_PASS )
  {
    return COSM_FAIL;
    CosmBNFree( &tmp );
  }

  tmp.digits = a->digits + b->digits;

  carry = 0;

  /* be sure a longer then b for speed */
  if ( a->digits < b->digits )
  {
    swap = a;
    a = b;
    b = swap;
  }

  /*
    Step through digits of b, and shift/add a each time.
  */
  for ( i = 0 ; i < b->digits ; i ++ )
  {
    m = b->n[i];
    pa = a->n;
    px = &tmp.n[i];
    for ( j = 0 ; j < a->digits ; j ++ )
    {
      /*
        ( x^n - 1 ) * ( x^n - 1 ) + ( x^n - 1 ) * x
        = x^(n*2) - 1
        so we can ( multiply + carry + add ) in any base.
      */
      carry += ( (COSM_BN_DWORD) *(pa++) * (COSM_BN_DWORD) m )
        + (COSM_BN_DWORD) *px;
      *(px++) = (COSM_BN_WORD) carry;
      carry >>= COSM_BN_BITS;
    }

    while ( carry )
    {
      carry += (COSM_BN_DWORD) *(px);
      *(px++) = (COSM_BN_WORD) carry;
      carry >>= COSM_BN_BITS;
    }
  }
#else
  /* do the math on half words instead */

  if ( CosmBNInit( &ta ) || CosmBNInit( &tb ) )
  {
    CosmBNFree( &tmp );
    return COSM_FAIL;
  }

  if ( ( CosmBNSet( &ta, a ) ) || ( CosmBNSet( &tb, b ) )
    || ( Cosm_BNExplode( &ta ) ) || ( Cosm_BNExplode( &tb ) )
    || ( Cosm_BNGrow( &tmp, ta.digits + tb.digits ) ) )
  {
    return COSM_FAIL;
  }

  tmp.digits = ta.digits + tb.digits;

  carry = 0;

  /*
    Step through digits of b, and shift/add a each time.
    Slightly faster if a < b
  */
  for ( i = 0 ; i < tb.digits ; i ++ )
  {
    m = tb.n[i];
    pa = ta.n;
    px = &tmp.n[i];
    for ( j = 0 ; j < ta.digits ; j ++ )
    {
      /*
        ( x^n - 1 ) * ( x^n - 1 ) + ( x^n - 1 ) * x
        = x^(n*2) - 1
        so we can ( multiply + carry + add ) in any base.
      */
      carry += ( *(pa++) * m ) + *px;
      *(px++) = carry & COSM_BN_MASK;
      carry >>= ( COSM_BN_BITS / 2 );
    }

    while ( carry )
    {
      carry += *(px);
      *(px++) = carry & COSM_BN_MASK;
      carry >>= ( COSM_BN_BITS / 2 );
    }
  }

  /* correct tmp.digits */
  while ( ( tmp.digits != 0 ) && ( tmp.n[tmp.digits - 1] == 0 ) )
  {
    tmp.digits--;
  }

  Cosm_BNImplode( &tmp );
  CosmBNFree( &ta );
  CosmBNFree( &tb );
#endif

  /* set correct x->digits */
  i = tmp.digits;
  px = &tmp.n[i-1];
  while ( ( i > 0 ) && ( *(px--) == 0 ) )
  {
    i--;
  }
  tmp.digits = i;

  ret = CosmBNSet( x, &tmp );
  CosmBNFree( &tmp );

  return ret;
}

s32 Cosm_BNDivMod( cosm_BN * x, cosm_BN * y, const cosm_BN * a, const cosm_BN * b )
{
  cosm_BN tdiv, tmod, tmp;
  u32 a_bits, b_bits, i;
  u32 div_sign;

  if ( CosmBNZero( a ) || CosmBNZero( b ) )
  {
    /* 0/x or x/0 = 0 */
    CosmBNSets32( x, 0 );
    CosmBNSets32( y, 0 );
    return COSM_PASS;
  }

  if ( CosmBNInit( &tdiv ) || CosmBNInit( &tmod ) || CosmBNInit( &tmp ) )
  {
    return COSM_FAIL;
  }

  div_sign = ( a->neg ^ b->neg );

  CosmBNSet( &tmod, a );
  tmod.neg = 0;
  if ( Cosm_BNuCmp( a, b ) >= 0 )
  {
    /* a >= b, do division, else just leave tdiv as 0, and tmod as a */
    CosmBNBits( &a_bits, a );
    CosmBNBits( &b_bits, b );

    CosmBNSet( &tmp, b );
    tmp.neg = 0;
    Cosm_BNLsh( &tmp, &tmp, a_bits - b_bits );
    i = a_bits - b_bits;
    Cosm_BNGrow( &tdiv, ( i / COSM_BN_BITS ) + 1 );
    for ( ; ; )
    {
      if ( Cosm_BNuCmp( &tmod, &tmp ) >= 0 )
      {
        Cosm_BNBitSet( &tdiv, i, 1 );
        Cosm_BNFastSub( &tmod, &tmp );
      }

      if ( i-- == 0 )
      {
        break;
      }

      Cosm_BNRsh1( &tmp );
    }
    /* tdiv = a/b, tmod = remainder */
  }

  if ( div_sign )
  {
    /* negate div */
    tdiv.neg = 1;

    /* normalize mod */
    tmod.neg = 1;
    if ( b->neg )
    {
      CosmBNSub( &tmod, &tmod, b );
    }
    else
    {
      CosmBNAdd( &tmod, &tmod, b );
    }
  }

  if ( x != NULL )
  {
    CosmBNSet( x, &tdiv );
  }
  if ( y != NULL )
  {
    CosmBNSet( y, &tmod );
  }
  CosmBNFree( &tdiv );
  CosmBNFree( &tmod );
  CosmBNFree( &tmp );

  return COSM_PASS;
}

s32 Cosm_BNuCmp( const cosm_BN * a, const cosm_BN * b )
{
  COSM_BN_WORD * pa, * pb;
  COSM_BN_SWORD i, result, d1;

  result = a->digits - b->digits;
  if ( result != 0 )
  {
    if ( result > 0 )
    {
      return 1;
    }
    else
    {
      return -1;
    }
  }

  /* same length */
  d1 = a->digits - 1;
  pa = &a->n[d1];
  pb = &b->n[d1];

  /* odds are high the high digit will be different */
  if ( *pa != *pb )
  {
    return ( *pa > *pb ) ? 1 : -1;
  }
  pa--;
  pb--;

  /* test the other digits */
  for ( i = d1 - 1 ; i >= 0 ; i-- )
  {
    if ( *pa != *pb )
    {
      return ( *pa > *pb ) ? 1 : -1;
    }
    pa--;
    pb--;
  }

  return COSM_PASS;
}

s32 Cosm_BNLsh( cosm_BN * x, const cosm_BN * a, const u32 s )
{
  cosm_BN tmp;
  u32 whole, part, npart;
  COSM_BN_SWORD i;
  COSM_BN_WORD * px;

  whole = s / COSM_BN_BITS;
  part = s % COSM_BN_BITS;

  if ( CosmBNInit( &tmp ) )
  {
    return COSM_FAIL;
  }

  if ( Cosm_BNGrow( &tmp, a->digits + whole + 1 ) )
  {
    CosmBNFree( &tmp );
    return COSM_FAIL;
  }

  tmp.digits = a->digits + whole + 1;

  if ( part == 0 )
  {
    for ( i = ( a->digits - 1 ) ; i >= 0 ; i-- )
    {
      tmp.n[i + whole] = a->n[i];
    }
  }
  else
  {
    npart = COSM_BN_BITS - part;
    for ( i = ( a->digits - 1 ) ; i >= 0 ; i-- )
    {
      tmp.n[i + whole + 1] |= a->n[i] >> npart;
      tmp.n[i + whole] = a->n[i] << part;
    }
  }

  /* set correct x->digits */
  i = tmp.digits;
  px = &tmp.n[i-1];
  while ( ( *(px--) == 0 ) && ( i > 0 ) )
  {
    i--;
  }
  tmp.digits = i;

  CosmBNSet( x, &tmp );
  CosmBNFree( &tmp );

  return COSM_PASS;
}

s32 Cosm_BNRsh1( cosm_BN * x )
{
  s32 d1;
  COSM_BN_SWORD i;
  COSM_BN_WORD * px;

  px = x->n;
  d1 = (u32) x->digits - 1;

  if ( d1 >= 0 )
  {
    /* shift each digit */
    for ( i = 0 ; i < (COSM_BN_SWORD) d1 ; i++ )
    {
      *px = ( *px >> 1 ) | ( px[1] << ( COSM_BN_BITS - 1 ) );
      px++;
    }

    /* shift the top digit */
    px = &x->n[d1];
    *px >>= 1;

    /* set correct x->digits */
    if ( *px == 0 )
    {
      x->digits--;
    }
  }

  return COSM_PASS;
}

s32 Cosm_BNBitSet( cosm_BN * x, u32 b, u32 value )
{
  u32 whole, part;
  COSM_BN_WORD *ptr;

  whole = b / COSM_BN_BITS;
  part = b % COSM_BN_BITS;
  ptr = &x->n[whole];

  if ( value )
  {
    /* set bit to 1 */
    *ptr |= ( (COSM_BN_WORD) 1 ) << part;
  }
  else
  {
    /* mask out bit */
    *ptr &= ~( ( (COSM_BN_WORD) 1 ) << part );
  }

  if ( x->digits < (COSM_BN_SWORD) ( whole + 1 ) )
  {
    x->digits = whole + 1;
  }

  return COSM_PASS;
}

/* testing */

#include "cosm/os_io.h"

s32 Cosm_TestBigNum( void )
{
  cosm_BN a, x, e, m;
  ascii str[32];
  s32 i;
  s32 error;
  u8 u8_a[32] =
  {
    0x56, 0xE1, 0xC4, 0x9E, 0xF2, 0x8F, 0x09, 0xF0,
    0x6F, 0xC1, 0x2F, 0xAF, 0x08, 0x54, 0x73, 0x39,
    0xB1, 0x6B, 0x72, 0x18, 0x7A, 0x38, 0xFB, 0x8B,
    0xD4, 0x86, 0x25, 0x2E, 0x02, 0xDA, 0x68, 0x42
  };
  u8 u8_e[32] =
  {
    0x5B, 0x4C, 0xD4, 0x67, 0x56, 0x86, 0x74, 0xC2,
    0x72, 0xDD, 0x82, 0xCE, 0x9E, 0x49, 0x2E, 0xE5,
    0xCF, 0x1E, 0x21, 0x20, 0xF9, 0xE5, 0x18, 0x8F,
    0x9F, 0xF5, 0x9A, 0x78, 0x7D, 0x75, 0x36, 0x51
  }; 
  u8 u8_m[32] =
  {
    0xB6, 0x99, 0xA8, 0xCE, 0xAD, 0x0C, 0xE9, 0x84,
    0xE5, 0xBB, 0x05, 0x9D, 0x3C, 0x92, 0x5D, 0xCB,
    0x9E, 0x3C, 0x42, 0x41, 0xF3, 0xCA, 0x31, 0x1F,
    0x3F, 0xEB, 0x34, 0xF0, 0xFA, 0xEA, 0x6C, 0xA3
  }; 
  u8 u8_x[32] =
  {
    0x6B, 0xA1, 0x33, 0xA2, 0x44, 0x14, 0x3D, 0xB3,
    0x95, 0xBC, 0x55, 0xD1, 0xA7, 0x2A, 0x34, 0xF0,
    0x3D, 0xF8, 0x56, 0xFF, 0x60, 0x1F, 0x70, 0x0C,
    0x8B, 0x1B, 0xF2, 0xEB, 0xD8, 0x99, 0x3F, 0x2B
  };

  if ( CosmBNInit( &a ) || CosmBNInit( &x )
    || CosmBNInit( &e ) || CosmBNInit( &m ) )
  {
    return -1;
  }

  error = COSM_PASS;

  /* test 668^79 mod 3337 = 1570 */
  if ( CosmBNSets32( &a, 688 ) || CosmBNSets32( &e, 79 )
    || CosmBNSets32( &m, 3337 ) )
  {
    error = -2;
    goto testbignum_error;
  }

  if ( CosmBNModExp( &x, &a, &e, &m ) != COSM_PASS )
  {
    error = -3;
    goto testbignum_error;
  }

  if ( CosmBNGets32( &i, &x ) != COSM_PASS )
  {
    error = -4;
    goto testbignum_error;
  }

  if ( i != 1570 )
  {
    error = -5;
    goto testbignum_error;
  }

  if ( CosmBNPrint( NULL, str, (u64) 1024, 10, &x ) != 4LL )
  {
    error = -6;
    goto testbignum_error;
  }

  if ( CosmStrCmp( str, "1570", 32LL ) != 0 )
  {
    error = -7;
    goto testbignum_error;
  }

  /* test 79^(-1) mod 3220 = 1019 */

  if ( CosmBNSets32( &a, 79 ) || CosmBNSets32( &m, 3220 ) )
  {
    error = -8;
    goto testbignum_error;
  }

  if ( CosmBNModInv( &x, &a, &m ) != COSM_PASS )
  {
    error = -9;
    goto testbignum_error;
  }

  if ( CosmBNGets32( &i, &x ) != COSM_PASS )
  {
    error = -10;
    goto testbignum_error;
  }

  if ( i != 1019 )
  {
    error = -11;
    goto testbignum_error;
  }

  if ( CosmBNPrint( NULL, str, (u64) 1024, 10, &x ) != 4LL )
  {
    error = -12;
    goto testbignum_error;
  }

  if ( CosmStrCmp( str, "1019", 32LL ) != 0 )
  {
    error = -13;
    goto testbignum_error;
  }

  /* GCD 15, 28 = 1 */

  if ( CosmBNSets32( &a, 15 ) || CosmBNSets32( &m, 28 ) )
  {
    error = -14;
    goto testbignum_error;
  }

  if ( CosmBNGCD( &x, &a, &m ) || ( x.n[0] != 1 ) )
  {
    error = -15;
    goto testbignum_error;
  }

  /* GCD 15, 27 = 3 */

  if ( CosmBNSets32( &a, 15 ) || CosmBNSets32( &m, 27 ) )
  {
    error = -16;
    goto testbignum_error;
  }

  if ( CosmBNGCD( &x, &a, &m ) || ( x.n[0] != 3 ) )
  {
    error = -17;
    goto testbignum_error;
  }

  /* test a big ( x = a^e mod m ), use a for the known correct x */
  if ( CosmBNLoad( &a, u8_a, 256 )
    || CosmBNLoad( &e, u8_e, 256 )
    || CosmBNLoad( &m, u8_m, 256 )
    || CosmBNModExp( &x, &a, &e, &m )
    || CosmBNLoad( &a, u8_x, 256 )
    || ( CosmBNCmp( &x, &a ) != 0 ) )
  {
    error = -18;
  }
  
  /* save and load */

testbignum_error:
  CosmBNFree( &a );
  CosmBNFree( &x );
  CosmBNFree( &e );
  CosmBNFree( &m );
  return error;
}
