/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - CPU/OS Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2007 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

/* CPU/OS Layer - CPU and OS specific code is allowed */

#include "cosm/os_math.h"

/* bigger */

u128 CosmU128U64( u64 a )
{
  u128 r;

  r.hi = 0;
  r.lo = a;
  return r;
}

s128 CosmS128S64( s64 a )
{
  s128 r;

  if ( a < 0 )
  {
    r.hi = 0xFFFFFFFFFFFFFFFFLL;
    r.lo = a;
  }
  else
  {
    r.hi = 0;
    r.lo = a;
  }
  return r;
}

u128 CosmU128U32( u32 a )
{
  u128 r;

  r.hi = 0;
  r.lo = (u64) a;
  return r;
}

s128 CosmS128S32( s32 a )
{
  s128 r;

  if ( a < 0 )
  {
    r.hi = 0xFFFFFFFFFFFFFFFFLL;
    r.lo = a;
  }
  else
  {
    r.hi = 0;
    r.lo = a;
  }

  return r;
}

/* smaller - truncation will occur */

u128 CosmU128S128( s128 a )
{
  u128 r;

  r.hi = (u64) a.hi;
  r.lo = (u64) a.lo;

  return r;
}

s128 CosmS128U128( u128 a )
{
  s128 r;

  r.hi = (s64) a.hi;
  r.lo = (u64) a.lo;

  return r;
}

void CosmU64DivMod( u64 a, u64 b, u64 * div, u64 * mod )
{
  u64 result;
  u64 rest;
  u64 mask;
  u64 temp;
  int nb;

  result = 0;
  rest = a;
  mask = b;
  temp = ( 1LL << 63 );
  nb = 0;

  if ( ( ( b == 0 ) ) || ( ( a == 0 ) ) )
  {
    /* 0/x or x/0 - Return 0 */
    *(div) = 0;
    *(mod) = 0;
    return;
  }

  /* First left shift the 'b' so it have the same number of bit then 'a' */

  /* while ( ( a > mask ) && ( ( 64th bit of mask ) == 0 ) */
  while ( ( a > mask ) && ( ( temp & mask ) == 0 ) )
  {
    mask = ( mask << 1 );
    nb = nb + 1;
  }

  /* Now right shift step by step and compare rest and divisor */

  while ( nb > 0 )
  {
    if ( !( mask > rest ) )
    {
      /* rest >= mask */
      rest = ( rest - mask );
      result++;
    }
    result = ( result << 1 );
    mask = ( mask >> 1 );
    nb = nb - 1;
  }

  if ( !( mask > rest ) )
  {
    /* rest >= mask */
    rest = ( rest - mask );
    result++;
  }

  *(div) = result;
  *(mod) = rest;
}

/*
  u128 Functions
*/

u128 CosmU128Add( u128 a, u128 b )
{
  u128 r;

  r.lo = a.lo + b.lo;
  r.hi = a.hi + b.hi;
  if ( r.lo < a.lo ) /* Deal with the overflow in the lower order */
  {
    r.hi++;
  }

  return r;
}

u128 CosmU128Sub( u128 a, u128 b )
{
  /* c = a + ~b + 1 */
  return CosmU128Add( a, CosmU128Add( CosmU128Not( b ), ( CosmU128U32( 1 ) ) ) );
}

u128 CosmU128Mul( u128 a, u128 b )
{
  u128 r, tmp;
  u64 a0, a1, a2, a3, b0, b1, b2, b3;

  a0 = a.lo & 0xFFFFFFFF;
  a1 = ( a.lo >> 32 ) & 0xFFFFFFFF;
  a2 = a.hi & 0xFFFFFFFF;
  a3 = ( a.hi >> 32 ) & 0xFFFFFFFF;
  b0 = b.lo & 0xFFFFFFFF;
  b1 = ( b.lo >> 32 ) & 0xFFFFFFFF;
  b2 = b.hi & 0xFFFFFFFF;
  b3 = ( b.hi >> 32 ) & 0xFFFFFFFF;

  /* calculate subresults */
  /* efficiency = 10 mul64, 3 shift128, 9 add128, 10 promotes */

  r = CosmU128U64( a0 * b0 );

  tmp = CosmU128U64( a0 * b1 );
  tmp = CosmU128Add( tmp, CosmU128U64( a1 * b0 ) );
  r = CosmU128Add( r, CosmU128Lsh( tmp, (u32) 32 ) );

  tmp = CosmU128U64( a0 * b2 );
  tmp = CosmU128Add( tmp, CosmU128U64( a1 * b1 ) );
  tmp = CosmU128Add( tmp, CosmU128U64( a2 * b0 ) );
  r = CosmU128Add( r, CosmU128Lsh( tmp, (u32) 64 ) );

  tmp = CosmU128U64( a0 * b3 );
  tmp = CosmU128Add( tmp, CosmU128U64( a1 * b2 ) );
  tmp = CosmU128Add( tmp, CosmU128U64( a2 * b1 ) );
  tmp = CosmU128Add( tmp, CosmU128U64( a3 * b0 ) );
  r = CosmU128Add( r, CosmU128Lsh( tmp, (u32) 96 ) );

  return r;
}

u128 CosmU128Div( u128 a, u128 b )
{
  u128 div;
  u128 mod;

  CosmU128DivMod( a, b, &div, &mod );

  return div;
}

u128 CosmU128Mod( u128 a, u128 b )
{
  u128 div;
  u128 mod;

  CosmU128DivMod( a, b, &div, &mod );

  return mod;
}

u128 CosmU128Inc( u128 * a )
{
  u128 r;

  r = *a;
  *a = CosmU128Add( r, CosmU128U32( 1 ) );

  return r;
}

u128 CosmU128Dec( u128 * a )
{
  u128 r;

  r = *a;
  *a = CosmU128Sub( r, CosmU128U32( 1 ) );

  return r;
}

u128 CosmU128And( u128 a, u128 b )
{
  u128 r;

  r.hi = a.hi & b.hi;
  r.lo = a.lo & b.lo;

  return r;
}

u128 CosmU128Or ( u128 a, u128 b )
{
  u128 r;

  r.hi = a.hi | b.hi;
  r.lo = a.lo | b.lo;

  return r;
}

u128 CosmU128Xor( u128 a, u128 b )
{
  u128 r;

  r.hi = a.hi ^ b.hi;
  r.lo = a.lo ^ b.lo;

  return r;
}

u128 CosmU128Not( u128 a )
{
  u128 r;

  r.hi = ~a.hi;
  r.lo = ~a.lo;

  return r;
}

u128 CosmU128Lsh( u128 a, u32 x )
{
  u128 r;

  if ( x > 127 )
    return CosmU128U32( 0 );

  /* Take care of shifts by offset > 63 */
  if ( x <= 64 )
  {
    if ( x == 64 )
    {
      r.lo = 0;
      r.hi = a.lo;
    }
    else
    {
      r.hi = ( a.hi << x ) | ( a.lo >> (64 - x) );
      r.lo = a.lo << x;
    }
  }
  else
  {
    r.hi = a.lo << (x - 64);
    r.lo = 0;
  }

  return r;
}

u128 CosmU128Rsh( u128 a, u32 x )
{
  u128 r;

  if ( x > 127 )
  {
    return CosmU128U32( 0 );
  }

  /* Take care of shifts by offset > 63 */
  if ( x <= 64 )
  {
    if ( x == 64 )
    {
      r.lo = a.hi;
      r.hi = 0;
    }
    else
    {
      r.lo = ( a.hi << (64 - x) ) | (a.lo >> x);
      r.hi = a.hi >> x;
    }
  }
  else
  {
    r.lo = a.hi >> (x - 64);
    r.hi = 0;
  }

  return r;
}

u32 CosmU128Eq( u128 a, u128 b )
{
  return ( a.hi == b.hi ) && ( a.lo == b.lo );
}

u32 CosmU128Gt( u128 a, u128 b )
{
  if ( ( a.hi > b.hi ) || ( ( a.hi == b.hi ) && ( a.lo > b.lo ) ) )
  {
    return 1;
  }
  return 0;
}

u32 CosmU128Lt( u128 a, u128 b )
{
  if ( ( a.hi < b.hi ) || ( ( a.hi == b.hi ) && ( a.lo < b.lo ) ) )
  {
    return 1;
  }
  return 0;
}

/*
  s128 Functions
*/

s128 CosmS128Add( s128 a, s128 b )
{
  u128 uA, uB, uResult;
  s128 result;

  uA.hi = (u64) a.hi;
  uA.lo = (u64) a.lo;
  uB.hi = (u64) b.hi;
  uB.lo = (u64) b.lo;

  uResult = CosmU128Add( uA, uB );
  result.hi = (s64) uResult.hi;
  result.lo = (u64) uResult.lo;

  return result;
}

s128 CosmS128Sub( s128 a, s128 b )
{
  u128 uA, uB, uResult;
  s128 result;

  uA.hi = (u64) a.hi;
  uA.lo = (u64) a.lo;
  uB.hi = (u64) b.hi;
  uB.lo = (u64) b.lo;

  uResult = CosmU128Sub( uA, uB );
  result.hi = (s64) uResult.hi;
  result.lo = (u64) uResult.lo;

  return result;
}

s128 CosmS128Mul( s128 a, s128 b )
{
  u128 uA, uB, uR;
  s128 result;
  s32 sign;       /* 0 = no, else yes */

  uA.hi = (u64) a.hi;
  uA.lo = (u64) a.lo;
  uB.hi = (u64) b.hi;
  uB.lo = (u64) b.lo;

  sign = 0;
  /* Get the absolute value of a and b */
  if ( a.hi < 0 )
  {
    sign++;
    uA = CosmU128Add( CosmU128Not( uA ), CosmU128U32( 1 ) );
  }
  if ( b.hi < 0 )
  {
    sign--;
    uB = CosmU128Add( CosmU128Not( uB ), CosmU128U32( 1 ) );
  }

  uR = CosmU128Mul( uA, uB );

  if ( sign != 0 )
  {
    /* The result must be negative */
    uR = CosmU128Add( CosmU128Not( uR ), CosmU128U32( 1 ) );
  }
  result.hi = (s64) uR.hi;
  result.lo = (u64) uR.lo;

  return result;
}

s128 CosmS128Div( s128 a, s128 b )
{
  u8 sign;
  u128 uA, uB, uR;
  s128 result;

  sign = 0;
  uA.hi = (u64) a.hi;
  uA.lo = (u64) a.lo;
  uB.hi = (u64) b.hi;
  uB.lo = (u64) b.lo;

  /* remove the sign */
  /* result is negative if and only if one of a or b is negative */
  if ( a.hi < 0 )
  {
    sign++;
    /* uA = -a */
    uA = CosmU128Add( CosmU128Not( uA ), CosmU128U32( 1 ) );
  }
  if ( b.hi < 0 )
  {
    sign--;
    /* uB = -b */
    uB = CosmU128Add( CosmU128Not( uB ), CosmU128U32( 1 ) );
  }

  uR = CosmU128Div( uA, uB );

  /* Modify the sign if we need to */
  if ( sign != 0 )
  {
    /* result = -uR */
    uR = CosmU128Add( CosmU128Not( uR ), CosmU128U32( 1 ) );
  }

  result.hi = (s64) uR.hi;
  result.lo = (u64) uR.lo;

  return result;
}

s128 CosmS128Mod( s128 a, s128 b )
{
  u8 sign;
  u128 uA, uB, uR;
  s128 result;
  u128 zero;

  sign = 0;
  uA.hi = (u64) a.hi;
  uA.lo = (u64) a.lo;
  uB.hi = (u64) b.hi;
  uB.lo = (u64) b.lo;

  /* remove the sign */
  /* result is negative if and only if a is negative */
  if ( a.hi < 0 )
  {
    sign++;
    /* uA = -a */
    uA = CosmU128Add( CosmU128Not( uA ), CosmU128U32( 1 ) );
  }
  if ( b.hi < 0 )
  {
    sign--;
    /* uB = -b */
    uB = CosmU128Add( CosmU128Not( uB ), CosmU128U32( 1 ) );
  }

  uR = CosmU128Mod( uA, uB );

  /* Modify the sign if we need to */
  _COSM_SET128( zero, 0000000000000000, 0000000000000000 );
  if ( ( sign != 0 ) && ( !CosmU128Eq( uR, zero ) ) )
  {
    /* result = -uR */
    uR = CosmU128Add( CosmU128Not( uR ), CosmU128U32( 1 ) );
    uR = CosmU128Add( uR, uB );
  }

  result.hi = (s64) uR.hi;
  result.lo = (u64) uR.lo;

  return result;
}

void CosmU128DivMod( u128 a, u128 b, u128 * div, u128 * mod )
{
  u128 result;
  u128 rest;
  u128 mask;
  u128 temp;
  int nb;

  result = CosmU128U32( 0 );
  rest = a;
  mask = b;
  temp = CosmU128Lsh( CosmU128U32( 1 ), 127 );
  nb = 0;

  if ( ( CosmU128Eq( b, CosmU128U32( 0 ) ) )
    || ( CosmU128Eq( a, CosmU128U32( 0 ) ) ) )
  {
    /* 0/x or x/0 - Return 0 */
    *(div) = CosmU128U32( 0 );
    *(mod) = CosmU128U32( 0 );
    return;
  }

  /* First left shift the 'b' so it have the same number of bit then 'a' */

  /* while ( ( a > mask ) && ( ( 64th bit of mask ) == 0 ) */
  while ( CosmU128Gt( a, mask ) && ( CosmU128Eq( CosmU128And( temp, mask ),
    CosmU128U32( 0 ) ) ) )
  {
    mask = CosmU128Lsh( mask, 1 );
    nb = nb + 1;
  }

  /* Now right shift step by step and compare rest and divisor */

  while ( nb > 0 )
  {
    if ( !CosmU128Gt( mask, rest ) )
    {
      /* rest >= mask */
      rest = CosmU128Sub( rest, mask );
      CosmU128Inc( &result );
    }
    result = CosmU128Lsh( result, 1 );
    mask = CosmU128Rsh( mask, 1 );
    nb = nb - 1;
  }

  if ( !CosmU128Gt( mask, rest ) )
  {
    /* rest >= mask */
    rest = CosmU128Sub( rest, mask );
    CosmU128Inc( &result );
  }

  *(div) = result;
  *(mod) = rest;
}

s128 CosmS128Inc( s128 * a )
{
  s128 r;

  r = *a;
  *a = CosmS128Add( r, CosmS128S32( 1 ) );

  return r;
}

s128 CosmS128Dec( s128 * a )
{
  s128 r;

  r = *a;
  *a = CosmS128Sub( r, CosmS128S32( 1 ) );

  return r;
}

u32 CosmS128Eq( s128 a, s128 b )
{
  return ( a.hi == b.hi ) && ( a.lo == b.lo );
}

u32 CosmS128Gt( s128 a, s128 b )
{
  if ( ( a.hi > b.hi ) || ( ( a.hi == b.hi ) && ( a.lo > b.lo ) ) )
  {
    return 1;
  }
  return 0;
}

u32 CosmS128Lt( s128 a, s128 b )
{
  if ( ( a.hi < b.hi ) || ( ( a.hi == b.hi ) && ( a.lo < b.lo ) ) )
  {
    return 1;
  }
  return 0;
}

#if ( !defined( NO_FLOATING_POINT ) )
s32 CosmFloatNaN( f64 number )
{
  u32 * n;

  n = (void *) &number;

  /*
    NaN: u = dont care, At least one x-bit/X-byte must be non-zero.
    f64 = [u111]FFX XXXX XXXX XXXX
  */
#if ( COSM_ENDIAN_CURRENT == COSM_ENDIAN_BIG )
  return ( ( n[0] & 0x7FF00000 ) == 0x7FF00000 )
    && ( ( ( 0x000FFFFF & n[0] ) | n[1] ) != 0 );
#else
  return ( ( n[1] & 0x7FF00000 ) == 0x7FF00000 )
    && ( ( ( 0x000FFFFF & n[1] ) | n[0] ) != 0 );
#endif
}

s32 CosmFloatInf( f64 number )
{
  s32 * n;

  n = (void *) &number;

  /*
    +/- INF: s = sign bit.
    f64 = [s111]FF0 0000 0000 0000
  */
#if ( COSM_ENDIAN_CURRENT == COSM_ENDIAN_BIG )
  n[1] |= ( n[0] & 0x7fffffff ) ^ 0x7ff00000;
  n[1] |= -n[1];
  return ~( n[1] >> 31 ) & ( n[0] >> 30 );
#else
  n[0] |= ( n[1] & 0x7fffffff ) ^ 0x7ff00000;
  n[0] |= -n[0];
  return ~( n[0] >> 31 ) & ( n[1] >> 30 );
#endif
}
#endif

/* testing */

s32 Cosm_TestOSMath( void )
{
  u32   u_32a,  u_32b,  u_32r;
  s32   s_32a,  s_32b,  s_32r;
  u64   u_64a,  u_64b,  u_64r;
  s64   s_64a,  s_64b,  s_64r;
  u128 u_128a, u_128b, u_128r;
  s128 s_128a, s_128b, s_128r;

  /*
    Basic methods: Test each function for valid results. Be sure to test
    all border conditions - positive to negative, rollovers, 0, etc.
    a small (+/+), b large (+/-)
    Add: a+b/b+a ordering, a+a no carry, b+b carry
    Sub:

    Base test numbers - here for reference
    u_32a = 0x01234567;
    u_32b = 0xFEDC89AB;
    s_32a = 0x01234567;
    s_32b = 0xFEDC89AB;
    u_64a = 0x0123456789ABCDEFLL;
    u_64b = 0xFEDC89AB76543210LL;
    s_64a = 0x0123456789ABCDEFLL;
    s_64b = 0xFEDC89AB76543210LL;
    _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
    _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );
    _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
    _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );
  */

  /* Conversions bigger and smaller */

  u_32a = 0x01234567;
  u_32b = 0xFEDC89AB;
  s_32a = 0x01234567;
  s_32b = 0xFEDC89AB;
  u_64a = 0x0123456789ABCDEFULL;
  u_64b = 0xFEDC89AB76543210LL;
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  /* u32 -> u64 */
  u_64r = (u64) u_32a;
  if ( u_64r != 0x0000000001234567LL )
  {
    return -1;
  }
  u_64r = (u64) u_32b;
  if ( u_64r != 0x00000000FEDC89ABLL )
  {
    return -2;
  }

  /* s32 ->  s64 */
  s_64r = (s64) s_32a;
  if ( s_64r != 0x0000000001234567LL )
  {
    return -3;
  }
  s_64r = (s64) s_32b;
  if ( s_64r != 0xFFFFFFFFFEDC89ABLL )
  {
    return -4;
  }

  /* u64 -> u128 */
  u_128r = CosmU128U64( u_64a );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0123456789ABCDEF ) )
  {
    return -5;
  }
  u_128r = CosmU128U64( u_64b );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, FEDC89AB76543210 ) )
  {
    return -6;
  }

  /* s64 -> s128 */
  s_128r = CosmS128S64( s_64a );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0123456789ABCDEF ) )
  {
    return -7;
  }
  s_128r = CosmS128S64( s_64b );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FEDC89AB76543210 ) )
  {
    return -8;
  }

  /* u32 -> u128 */
  u_128r = CosmU128U32( u_32a );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000001234567 ) )
  {
    return -9;
  }
  u_128r = CosmU128U32( u_32b );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 00000000FEDC89AB ) )
  {
    return -10;
  }

  /* s32 -> s128 */
  s_128r = CosmS128S32( s_32a );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0000000001234567 ) )
  {
    return -11;
  }
  s_128r = CosmS128S32( s_32b );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFEDC89AB ) )
  {
    return -12;
  }

  /* u64 -> u32 */
  u_32r = (u32) u_64a;
  if ( u_32r != 0x89ABCDEF )
  {
    return -13;
  }
  u_32r = (u32) u_64b;
  if ( u_32r != 0x76543210 )
  {
    return -14;
  }

  /* s64 ->  s32 */
  s_32r = (s32) s_64a;
  if ( s_32r != (s32) 0x89ABCDEF )
  {
    return -15;
  }
  s_32r = (s32) s_64b;
  if ( s_32r != 0x76543210 )
  {
    return -16;
  }

  /* u128 -> u64 */
  u_64r = CosmU64U128( u_128a );
  if ( u_64r != 0x45670123CDEF89ABLL )
  {
    return -17;
  }
  u_64r = CosmU64U128( u_128b );
  if ( u_64r != 0x89ABFEDC32107654LL )
  {
    return -18;
  }

  /* s128 -> s64 */
  s_64r = CosmS64S128( s_128a );
  if ( s_64r != 0x45670123CDEF89ABLL )
  {
    return -19;
  }
  s_64r = CosmS64S128( s_128b );
  if ( s_64r != 0x89ABFEDC32107654LL )
  {
    return -20;
  }

  /* u128 -> u32 */
  u_32r = CosmU32U128( u_128a );
  if ( u_32r != 0xCDEF89AB )
  {
    return -21;
  }
  u_32r = CosmU32U128( u_128b );
  if ( u_32r != 0x32107654 )
  {
    return -22;
  }

  /* s128 -> s32 */
  s_32r = CosmS32S128( s_128a );
  if ( s_32r != (s32) 0xCDEF89AB )
  {
    return -23;
  }
  s_32r = CosmS32S128( s_128b );
  if ( s_32r != (s32) 0x32107654 )
  {
    return -24;
  }

  /* u64 functions ====================================================== */

  /* u64 Add */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;

  u_64r = ( u_64a + u_64b );
  if ( u_64r != 0xFFFFCF12FFFFFFFFLL )
  {
    return -25;
  }
  u_64r = ( u_64b + u_64a );
  if ( u_64r != 0xFFFFCF12FFFFFFFFLL )
  {
    return -26;
  }
  u_64r = ( u_64a + u_64a );
  if ( u_64r != 0x02468ACF13579BDELL )
  {
    return -27;
  }
  u_64r = ( u_64b + u_64b );
  if ( u_64r != 0xFDB91356ECA86420LL )
  {
    return -28;
  }

  /* u64 Sub */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;

  u_64r = ( u_64a - u_64b );
  if ( u_64r != 0x0246BBBC13579BDFLL )
  {
    return -29;
  }

  u_64r = ( u_64b - u_64a );
  if ( u_64r != 0xFDB94443ECA86421LL )
  {
    return -30;
  }

  u_64r = ( u_64a - u_64a );
  if ( u_64r != 0x0000000000000000LL )
  {
    return -31;
  }

  u_64r = ( u_64b - u_64b );
  if ( u_64r != 0x0000000000000000LL )
  {
    return -32;
  }

  /* u64 Mul */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;

  u_64r = ( u_64a * u_64a );
  if ( u_64r != 0xDCA5E20890F2A521LL )
  {
    return -33;
  }

  u_64r = ( u_64b * u_64b );
  if ( u_64r != 0x3441BB37A44A4100LL )
  {
    return -34;
  }

  u_64r = ( u_64a * u_64b );
  if ( u_64r != 0x778C624CE5618CF0LL )
  {
    return -35;
  }

  u_64r = ( u_64b * u_64a );
  if ( u_64r != 0x778C624CE5618CF0LL )
  {
    return -36;
  }

  /* u64 Div, Mod */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;

  u_64r = ( u_64a / u_64b );
  if ( u_64r != 0x0000000000000000LL )
  {
    return -37;
  }

  u_64r = ( u_64a % u_64b );
  if ( u_64r != 0x0123456789ABCDEFLL )
  {
    return -38;
  }

  u_64r = ( u_64b / u_64a );
  if ( u_64r != 0x00000000000000DFLL )
  {
    return -39;
  }

  u_64r = ( u_64b % u_64a );
  if ( u_64r != 0x0123147A89ABCEDFLL )
  {
    return -40;
  }

  /* u64 Inc */
  u_64r = 0xFFFFFFFFFFFFFFFELL;

  u_64r++;
  if ( u_64r != 0xFFFFFFFFFFFFFFFFLL )
  {
    return -41;
  }
  u_64r++;
  if ( u_64r != 0x0000000000000000LL )
  {
    return -42;
  }

  /* u64 Dec */
  u_64r--;
  if ( u_64r != 0xFFFFFFFFFFFFFFFFLL )
  {
    return -43;
  }
  u_64r--;
  if ( u_64r != 0xFFFFFFFFFFFFFFFELL )
  {
    return -44;
  }

  /* u64 And */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;

  u_64r = ( u_64a & u_64a );
  if ( u_64r != 0x0123456789ABCDEFLL )
  {
    return -45;
  }
  u_64r = ( u_64a & u_64b );
  if ( u_64r != 0x0000012300000000LL )
  {
    return -46;
  }

  /* u64 Or */
  u_64r = ( u_64a | u_64a );
  if ( u_64r != 0x0123456789ABCDEFLL )
  {
    return -47;
  }
  u_64r = ( u_64a | u_64b );
  if ( u_64r != 0xFFFFCDEFFFFFFFFFLL )
  {
    return -48;
  }

  /* u64 Xor */
  /* r = a ^ b ^ a ^ b ^ a = a */
  u_64r = ( u_64a ^ u_64b );
  u_64r = ( u_64r ^ u_64a );
  u_64r = ( u_64r ^ u_64b );
  u_64r = ( u_64r ^ u_64a );
  if ( u_64r != 0x0123456789ABCDEFLL )
  {
    return -49;
  }

  /* u64 Not */
  u_64r = ( ~u_64a );
  if ( u_64r != 0xFEDCBA9876543210LL )
  {
    return -50;
  }
  u_64r = ( ~u_64b );
  if ( u_64r != 0x0123765489ABCDEFLL )
  {
    return -51;
  }

  /* u64 Lsh */
  u_64r = ( u_64a << 4 );
  if ( u_64r != 0x123456789ABCDEF0LL )
  {
    return -52;
  }
  u_64r = ( u_64a << 36 );
  if ( u_64r != 0x9ABCDEF000000000LL )
  {
    return -53;
  }

  /* u64 Rsh */
  u_64r = ( u_64a >> 4 );
  if ( u_64r != 0x00123456789ABCDELL )
  {
    return -54;
  }
  u_64r = ( u_64a >> 36 );
  if ( u_64r != 0x0000000000123456LL )
  {
    return -55;
  }

  /* u64 Eq */
  u_64a = 0x0123456789ABCDEFLL;
  u_64b = 0xFEDC89AB76543210LL;
  if ( u_64a != u_64a )
  {
    return -56;
  }
  if ( u_64b != u_64b )
  {
    return -57;
  }
  if ( ( u_64a == u_64b ) )
  {
    return -58;
  }
  if ( ( u_64b == u_64a ) )
  {
    return -59;
  }

  /* u64 Gt */
  if ( ( u_64a > u_64a ) )
  {
    return -60;
  }
  if ( ( u_64b > u_64b ) )
  {
    return -61;
  }
  if ( ( u_64a > u_64b ) )
  {
    return -62;
  }
  if ( !( u_64b > u_64a ) )
  {
    return -63;
  }

  /* u64 Lt */
  if ( ( u_64a < u_64a ) )
  {
    return -64;
  }
  if ( ( u_64b < u_64b ) )
  {
    return -65;
  }
  if ( !( u_64a < u_64b ) )
  {
    return -66;
  }
  if ( ( u_64b < u_64a ) )
  {
    return -67;
  }

  /* s64 functions ====================================================== */

  /* s64 Add */
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;

  s_64r = ( s_64a + s_64b );
  if ( s_64r != 0xFFFFCF12FFFFFFFFLL )
  {
    return -68;
  }
  s_64r = ( s_64b + s_64a );
  if ( s_64r != 0xFFFFCF12FFFFFFFFLL )
  {
    return -69;
  }
  s_64r = ( s_64a + s_64a );
  if ( s_64r != 0x02468ACF13579BDELL )
  {
    return -70;
  }
  s_64r = ( s_64b + s_64b );
  if ( s_64r != 0xFDB91356ECA86420LL )
  {
    return -71;
  }

  /* s64Sub */
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;

  s_64r = ( s_64a - s_64b );
  if ( s_64r != 0x0246BBBC13579BDFLL )
  {
    return -72;
  }

  s_64r = ( s_64b - s_64a );
  if ( s_64r != 0xFDB94443ECA86421LL )
  {
    return -73;
  }

  s_64r = ( s_64a - s_64a );
  if ( s_64r != 0x0000000000000000LL )
  {
    return -74;
  }

  s_64r = ( s_64b - s_64b );
  if ( s_64r != 0x0000000000000000LL )
  {
    return -75;
  }

  /* s64 Mul */
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;

  s_64r = ( s_64a * s_64a );
  if ( s_64r != 0xDCA5E20890F2A521LL )
  {
    return -76;
  }

  s_64r = ( s_64b * s_64b );
  if ( s_64r != 0x3441BB37A44A4100LL )
  {
    return -77;
  }

  s_64r = ( s_64a * s_64b );
  if ( s_64r != 0x778C624CE5618CF0LL )
  {
    return -78;
  }

  s_64r = ( s_64b * s_64a );
  if ( s_64r != 0x778C624CE5618CF0LL )
  {
    return -79;
  }

  /* s64 Div, Mod */
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;

  s_64r = ( s_64a / s_64b );
  if ( s_64r != 0x0000000000000000LL )
  {
    return -80;
  }

#if 0 /* !!! */
  s_64r = ( s_64a % s_64b );
  if ( s_64r != 0x000030ED00000001LL )
  {
    return -81;
  }
#endif

  s_64r = ( s_64b / s_64a );
  if ( s_64r != 0xFFFFFFFFFFFFFFFFLL )
  {
    return -82;
  }

#if 0 /* !!! */
  s_64r = ( s_64b % s_64a );
  if ( s_64r != 0x0123147A89ABCDEELL )
  {
    return -83;
  }
#endif

  /* s64 Inc */
  s_64r = 0xFFFFFFFFFFFFFFFELL;

  s_64r++;
  if ( s_64r != 0xFFFFFFFFFFFFFFFFLL )
  {
    return -84;
  }
  s_64r++;
  if ( s_64r != 0x0000000000000000LL )
  {
    return -85;
  }

  /* s64 Dec */
  s_64r--;
  if ( s_64r != 0xFFFFFFFFFFFFFFFFLL )
  {
    return -86;
  }
  s_64r--;
  if ( s_64r != 0xFFFFFFFFFFFFFFFELL )
  {
    return -87;
  }

  /* s64 Eq */
  s_64a = 0x0123456789ABCDEFLL;
  s_64b = 0xFEDC89AB76543210LL;

  if ( s_64a != s_64a )
  {
    return -88;
  }
  if ( s_64b != s_64b )
  {
    return -89;
  }
  if ( ( s_64a == s_64b ) )
  {
    return -90;
  }
  if ( ( s_64b == s_64a ) )
  {
    return -91;
  }

  /* s64 Gt */
  if ( ( s_64a > s_64a ) )
  {
    return -92;
  }
  if ( ( s_64b > s_64b ) )
  {
    return -93;
  }
  if ( !( s_64a > s_64b ) )
  {
    return -94;
  }
  if ( ( s_64b > s_64a ) )
  {
    return -95;
  }

  /* s64 Lt */
  if ( ( s_64a < s_64a ) )
  {
    return -96;
  }
  if ( ( s_64b < s_64b ) )
  {
    return -97;
  }
  if ( ( s_64a < s_64b ) )
  {
    return -98;
  }
  if ( !( s_64b < s_64a ) )
  {
    return -99;
  }

  /* u128 functions ===================================================== */

  /* u128 Add */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  u_128r = CosmU128Add( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, FFFFCF12FFFFFFFF, CF12FFFFFFFFFFFF ) )
  {
    return -100;
  }
  u_128r = CosmU128Add( u_128b, u_128a );
  if ( !_COSM_EQ128( u_128r, FFFFCF12FFFFFFFF, CF12FFFFFFFFFFFF ) )
  {
    return -101;
  }
  u_128r = CosmU128Add( u_128a, u_128a );
  if ( !_COSM_EQ128( u_128r, 02468ACF13579BDE, 8ACE02479BDF1356 ) )
  {
    return -102;
  }
  u_128r = CosmU128Add( u_128b, u_128b );
  if ( !_COSM_EQ128( u_128r, FDB91356ECA86421, 1357FDB86420ECA8 ) )
  {
    return -103;
  }

  /* u128 Sub */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  u_128r = CosmU128Sub( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, 0246BBBC13579BDE, BBBB02479BDF1357 ) )
  {
    return -104;
  }

  u_128r = CosmU128Sub( u_128b, u_128a );
  if ( !_COSM_EQ128( u_128r, FDB94443ECA86421, 4444FDB86420ECA9 ) )
  {
    return -105;
  }

  u_128r = CosmU128Sub( u_128a, u_128a );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -106;
  }

  u_128r = CosmU128Sub( u_128b, u_128b );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -107;
  }

  /* u128 Mul */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  u_128r = CosmU128Mul( u_128a, u_128a );
  if ( !_COSM_EQ128( u_128r, 4AF09DB050050EDE, C9D67D6035527839 ) )
  {
    return -108;
  }

  u_128r = CosmU128Mul( u_128b, u_128b );
  if ( !_COSM_EQ128( u_128r, 7EC36D85ECE1CBE2, BD1C7FA7D1318B90 ) )
  {
    return -109;
  }

  u_128r = CosmU128Mul( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, 9FD30906618C929F, 6D73817BFCBDFE1C ) )
  {
    return -110;
  }

  u_128r = CosmU128Mul( u_128b, u_128a );
  if ( !_COSM_EQ128( u_128r, 9FD30906618C929F, 6D73817BFCBDFE1C ) )
  {
    return -111;
  }

  /* u128 Div, Mod */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  u_128r = CosmU128Div( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -112;
  }

  u_128r = CosmU128Mod( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, 0123456789ABCDEF, 45670123CDEF89AB ) )
  {
    return -113;
  }

  u_128r = CosmU128Div( u_128b, u_128a );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 00000000000000DF ) )
  {
    return -114;
  }

  u_128r = CosmU128Mod( u_128b, u_128a );
  if ( !_COSM_EQ128( u_128r, 0123147A89ABCEA3, 14F200ABCE678A5F ) )
  {
    return -115;
  }

  /* u128 Inc */
  _COSM_SET128( u_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFE );

  CosmU128Inc( &u_128r );
  if ( !_COSM_EQ128( u_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF ) )
  {
    return -116;
  }
  CosmU128Inc( &u_128r );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -117;
  }

  /* u128 Dec */
  CosmU128Dec( &u_128r );
  if ( !_COSM_EQ128( u_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF ) )
  {
    return -118;
  }
  CosmU128Dec( &u_128r );
  if ( !_COSM_EQ128( u_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFE ) )
  {
    return -119;
  }

  /* u128 And */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  u_128r = CosmU128And( u_128a, u_128a );
  if ( !_COSM_EQ128( u_128r, 0123456789ABCDEF, 45670123CDEF89AB ) )
  {
    return -120;
  }
  u_128r = CosmU128And( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, 0000012300000000, 0123000000000000 ) )
  {
    return -121;
  }

  /* u128 Or */
  u_128r = CosmU128Or( u_128a, u_128a );
  if ( !_COSM_EQ128( u_128r, 0123456789ABCDEF, 45670123CDEF89AB ) )
  {
    return -122;
  }
  u_128r = CosmU128Or( u_128a, u_128b );
  if ( !_COSM_EQ128( u_128r, FFFFCDEFFFFFFFFF, CDEFFFFFFFFFFFFF ) )
  {
    return -123;
  }

  /* u128 Xor */
  /* r = a ^ b ^ a ^ b ^ a = a */
  u_128r = CosmU128Xor( u_128a, u_128b );
  u_128r = CosmU128Xor( u_128r, u_128a );
  u_128r = CosmU128Xor( u_128r, u_128b );
  u_128r = CosmU128Xor( u_128r, u_128a );
  if ( !_COSM_EQ128( u_128r, 0123456789ABCDEF, 45670123CDEF89AB ) )
  {
    return -124;
  }

  /* u128 Not */
  u_128r = CosmU128Not( u_128a );
  if ( !_COSM_EQ128( u_128r, FEDCBA9876543210, BA98FEDC32107654 ) )
  {
    return -125;
  }
  u_128r = CosmU128Not( u_128b );
  if ( !_COSM_EQ128( u_128r, 0123765489ABCDEF, 76540123CDEF89AB ) )
  {
    return -126;
  }

  /* u128 Lsh */
  u_128r = CosmU128Lsh( u_128a, 4 );
  if ( !_COSM_EQ128( u_128r, 123456789ABCDEF4, 5670123CDEF89AB0 ) )
  {
    return -127;
  }
  u_128r = CosmU128Lsh( u_128a, 36 );
  if ( !_COSM_EQ128( u_128r, 9ABCDEF45670123C, DEF89AB000000000 ) )
  {
    return -128;
  }
  u_128r = CosmU128Lsh( u_128a, 68 );
  if ( !_COSM_EQ128( u_128r, 5670123CDEF89AB0, 0000000000000000 ) )
  {
    return -129;
  }
  u_128r = CosmU128Lsh( u_128a, 100 );
  if ( !_COSM_EQ128( u_128r, DEF89AB000000000, 0000000000000000 ) )
  {
    return -130;
  }

  /* 128 Rsh */
  u_128r = CosmU128Rsh( u_128a, 4 );
  if ( !_COSM_EQ128( u_128r, 00123456789ABCDE, F45670123CDEF89A ) )
  {
    return -131;
  }
  u_128r = CosmU128Rsh( u_128a, 36 );
  if ( !_COSM_EQ128( u_128r, 0000000000123456, 789ABCDEF4567012 ) )
  {
    return -132;
  }
  u_128r = CosmU128Rsh( u_128a, 68 );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 00123456789ABCDE ) )
  {
    return -133;
  }
  u_128r = CosmU128Rsh( u_128a, 100 );
  if ( !_COSM_EQ128( u_128r, 0000000000000000, 0000000000123456 ) )
  {
    return -134;
  }

  /* u128 Eq */
  _COSM_SET128( u_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( u_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  if ( !CosmU128Eq( u_128a, u_128a ) )
  {
    return -135;
  }
  if ( !CosmU128Eq( u_128b, u_128b ) )
  {
    return -136;
  }
  if ( CosmU128Eq( u_128a, u_128b ) )
  {
    return -137;
  }
  if ( CosmU128Eq( u_128b, u_128a ) )
  {
    return -138;
  }

  /* u128 Gt */
  if ( CosmU128Gt( u_128a, u_128a ) )
  {
    return -139;
  }
  if ( CosmU128Gt( u_128b, u_128b ) )
  {
    return -140;
  }
  if ( CosmU128Gt( u_128a, u_128b ) )
  {
    return -141;
  }
  if ( !CosmU128Gt( u_128b, u_128a ) )
  {
    return -142;
  }

  /* u128 Lt */
  if ( CosmU128Lt( u_128a, u_128a ) )
  {
    return -143;
  }
  if ( CosmU128Lt( u_128b, u_128b ) )
  {
    return -144;
  }
  if ( !CosmU128Lt( u_128a, u_128b ) )
  {
    return -145;
  }
  if ( CosmU128Lt( u_128b, u_128a ) )
  {
    return -146;
  }

  /* s128 functions ===================================================== */

  /* s128 Add */
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  s_128r = CosmS128Add( s_128a, s_128b );
  if ( !_COSM_EQ128( s_128r, FFFFCF12FFFFFFFF, CF12FFFFFFFFFFFF ) )
  {
    return -147;
  }
  s_128r = CosmS128Add( s_128b, s_128a );
  if ( !_COSM_EQ128( s_128r, FFFFCF12FFFFFFFF, CF12FFFFFFFFFFFF ) )
  {
    return -148;
  }
  s_128r = CosmS128Add( s_128a, s_128a );
  if ( !_COSM_EQ128( s_128r, 02468ACF13579BDE, 8ACE02479BDF1356 ) )
  {
    return -149;
  }
  s_128r = CosmS128Add( s_128b, s_128b );
  if ( !_COSM_EQ128( s_128r, FDB91356ECA86421, 1357FDB86420ECA8 ) )
  {
    return -150;
  }

  /* u128 Sub */
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  s_128r = CosmS128Sub( s_128a, s_128b );
  if ( !_COSM_EQ128( s_128r, 0246BBBC13579BDE, BBBB02479BDF1357 ) )
  {
    return -151;
  }

  s_128r = CosmS128Sub( s_128b, s_128a );
  if ( !_COSM_EQ128( s_128r, FDB94443ECA86421, 4444FDB86420ECA9 ) )
  {
    return -152;
  }

  s_128r = CosmS128Sub( s_128a, s_128a );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -153;
  }

  s_128r = CosmS128Sub( s_128b, s_128b );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -154;
  }

  /* s128 Mul */
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  s_128r = CosmS128Mul( s_128a, s_128a );
  if ( !_COSM_EQ128( s_128r, 4AF09DB050050EDE, C9D67D6035527839 ) )
  {
    return -155;
  }

  s_128r = CosmS128Mul( s_128b, s_128b );
  if ( !_COSM_EQ128( s_128r, 7EC36D85ECE1CBE2, BD1C7FA7D1318B90 ) )
  {
    return -156;
  }

  s_128r = CosmS128Mul( s_128a, s_128b );
  if ( !_COSM_EQ128( s_128r, 9FD30906618C929F, 6D73817BFCBDFE1C ) )
  {
    return -157;
  }

  s_128r = CosmS128Mul( s_128b, s_128a );
  if ( !_COSM_EQ128( s_128r, 9FD30906618C929F, 6D73817BFCBDFE1C ) )
  {
    return -158;
  }

  /* s128 Div, Mod */
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );

  s_128r = CosmS128Div( s_128a, s_128b );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -159;
  }

  s_128r = CosmS128Mod( s_128a, s_128b );
  if ( !_COSM_EQ128( s_128r, 000030ED00000000, 30ED000000000001 ) )
  {
    return -160;
  }

  s_128r = CosmS128Div( s_128b, s_128a );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF ) )
  {
    return -161;
  }

  s_128r = CosmS128Mod( s_128b, s_128a );
  if ( !_COSM_EQ128( s_128r, 0123147A89ABCDEF, 147A0123CDEF89AA ) )
  {
    return -162;
  }

  /* s128 Inc */
  _COSM_SET128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFE );

  CosmS128Inc( &s_128r );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF ) )
  {
    return -163;
  }
  CosmS128Inc( &s_128r );
  if ( !_COSM_EQ128( s_128r, 0000000000000000, 0000000000000000 ) )
  {
    return -164;
  }

  /* s128 Dec */
  CosmS128Dec( &s_128r );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF ) )
  {
    return -165;
  }
  CosmS128Dec( &s_128r );
  if ( !_COSM_EQ128( s_128r, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFE ) )
  {
    return -166;
  }

  /* s128 Eq */
  _COSM_SET128( s_128a, 0123456789ABCDEF, 45670123CDEF89AB );
  _COSM_SET128( s_128b, FEDC89AB76543210, 89ABFEDC32107654 );
  if ( !CosmS128Eq( s_128a, s_128a ) )
  {
    return -167;
  }
  if ( !CosmS128Eq( s_128b, s_128b ) )
  {
    return -168;
  }
  if ( CosmS128Eq( s_128a, s_128b ) )
  {
    return -169;
  }
  if ( CosmS128Eq( s_128b, s_128a ) )
  {
    return -170;
  }

  /** compare ** Gt */
  if ( CosmS128Gt( s_128a, s_128a ) )
  {
    return -171;
  }
  if ( CosmS128Gt( s_128b, s_128b ) )
  {
    return -172;
  }
  if ( !CosmS128Gt( s_128a, s_128b ) )
  {
    return -173;
  }
  if ( CosmS128Gt( s_128b, s_128a ) )
  {
    return -174;
  }

  /* s128 Lt */
  if ( CosmS128Lt( s_128a, s_128a ) )
  {
    return -175;
  }
  if ( CosmS128Lt( s_128b, s_128b ) )
  {
    return -176;
  }
  if ( CosmS128Lt( s_128a, s_128b ) )
  {
    return -177;
  }
  if ( !CosmS128Lt( s_128b, s_128a ) )
  {
    return -178;
  }

  return COSM_PASS;
}
