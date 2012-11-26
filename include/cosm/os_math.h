/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - CPU/OS Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

/* CPU/OS Layer - CPU and OS specific code is allowed */

#ifndef COSM_OS_MATH_H
#define COSM_OS_MATH_H

#include "cosm/cputypes.h"

/* include the ANSI math function headers */
#if ( !defined( NO_FLOATING_POINT ) )
#include <math.h>
#endif

/* Make sure that PI is defined */
#if ( !defined( M_PI ) )
#define M_PI 3.1415926535897932384626433832795 /* pi */
#endif

/*
  Define arithmatic (+ - * / % ++ --) logical (& | ^ ~ << >>),
  and comparison ( == > < ) for u64, s64, u128, and s128
  Portability and clarity are important here, not speed.
  We do not need logical operations for signed values, use a typecase
  to unsigned value.
  Note: != <= >= aren't needed, that would just reverse the if statement.
*/

/*
  Convertions
*/

/* bigger <- smaller */
u128 CosmU128U64( u64 a ); /* u128 = u64 */
s128 CosmS128S64( s64 a ); /* s128 = s64 */
u128 CosmU128U32( u32 a ); /* u128 = u32 */
s128 CosmS128S32( s32 a ); /* s128 = s32 */

/* smaller <- bigger - truncation will occur */
#define CosmU64U128( a ) ( (u64) a.lo )
#define CosmS64S128( a ) ( (s64) a.lo )
#define CosmU32U128( a ) ( (u32) ( a.lo & 0xFFFFFFFF ) )
#define CosmS32S128( a ) ( (s32) ( a.lo & 0xFFFFFFFF ) )

/* signed <-> unsigned, sign may change */
u128 CosmU128S128( s128 a ); /* u128 <- s128 */
s128 CosmS128U128( u128 a ); /* s128 <- u128 */

/*
  u64 Functions
*/

void CosmU64DivMod( u64 a, u64 b, u64 * div, u64 * mod );
  /* div = a / b; mod = a % b; */

/*
  u128 Functions
*/

u128 CosmU128Add( u128 a, u128 b );  /* c = a + b */
u128 CosmU128Sub( u128 a, u128 b );  /* c = a - b */
u128 CosmU128Mul( u128 a, u128 b );  /* c = a * b */
u128 CosmU128Div( u128 a, u128 b );  /* c = a / b */
u128 CosmU128Mod( u128 a, u128 b );  /* c = a % b */
void CosmU128DivMod( u128 a, u128 b, u128 * div, u128 * mod );
                                   /* div = a / b; mod = a % b; */
u128 CosmU128Inc( u128 * a );        /* (*a)++ */
u128 CosmU128Dec( u128 * a );        /* (*a)-- */

u128 CosmU128And( u128 a, u128 b );  /* c = a & b */
u128 CosmU128Or ( u128 a, u128 b );  /* c = a | b */
u128 CosmU128Xor( u128 a, u128 b );  /* c = a ^ b */
u128 CosmU128Not( u128 a );          /* c = ^a */
u128 CosmU128Lsh( u128 a, u32 x );   /* c = a << x */
u128 CosmU128Rsh( u128 a, u32 x );   /* c = a >> x */

u32 CosmU128Eq( u128 a, u128 b );    /* a == b ? 1 : 0; */
u32 CosmU128Gt( u128 a, u128 b );    /* a > b ? 1 : 0; */
u32 CosmU128Lt( u128 a, u128 b );    /* a < b ? 1 : 0; */

/*
  s128 Functions
*/

s128 CosmS128Add( s128 a, s128 b );  /* c = a + b */
s128 CosmS128Sub( s128 a, s128 b );  /* c = a - b */
s128 CosmS128Mul( s128 a, s128 b );  /* c = a * b */
s128 CosmS128Div( s128 a, s128 b );  /* c = a / b */
s128 CosmS128Mod( s128 a, s128 b );  /* c = a % b */
s128 CosmS128Inc( s128 * a );        /* (*a)++ */
s128 CosmS128Dec( s128 * a );        /* (*a)-- */

u32 CosmS128Eq( s128 a, s128 b );    /* a == b ? 1 : 0; */
u32 CosmS128Gt( s128 a, s128 b );    /* a > b ? 1 : 0; */
u32 CosmS128Lt( s128 a, s128 b );    /* a < b ? 1 : 0; */

/* floating point NaN and Inf testing */

#if ( !defined( NO_FLOATING_POINT ) )
s32 CosmFloatNaN( f64 number );
  /*
    Test if number is NAN.
    Returns: 1 if number is NAN, 0 otherwise.
  */

s32 CosmFloatInf( f64 number );
  /*
    Test if number is +/- Inf.
    Returns: 1 if number is +Inf, -1 if -Inf, 0 otherwise.
  */
#endif

/* testing */

s32 Cosm_TestOSMath( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
