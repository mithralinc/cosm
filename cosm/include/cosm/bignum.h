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

#ifndef COSM_BN_H
#define COSM_BN_H

#include "cosm/cputypes.h"
#include "cosm/os_mem.h"
#include "cosm/os_file.h"

/*
  On a 32/64-bit CPU we can use 32 or 64bit words, larger is faster. Neither
  length nor top will ever need the type conversion macros used for math.
  However, defines may be needed if a u64 or u32 is specificly required. We
  limit the numbers to 2^31 - 64 bits in length (in order to do Mul),
  however most platforms will have much lower limit imposed by CosmMemAlloc.
*/

#define COSM_BN_MAXBITS 0x7FFFFFC0 /* 2^31 - 64 */

#undef COSM_BN_DWORD
#if ( defined( CPU_64BIT ) )
#define COSM_BN_WORD    u64
#define COSM_BN_SWORD   s64
#define COSM_BN_BITS    64
#define COSM_BN_MASK    0xFFFFFFFF
#else /* 32-bit system */
/* we have a compiler u64 we can use for add/mul that should be faster */
#define COSM_BN_DWORD   u64
#define COSM_BN_WORD    u32
#define COSM_BN_SWORD   s32
#define COSM_BN_BITS    32
#define COSM_BN_MASK    0xFFFF
#endif

#define COSM_BN_BYTES   ( COSM_BN_BITS / 8 )
#define COSM_BN_WLIMIT  ( COSM_BN_MAXBITS / COSM_BN_BITS )

#define COSM_BN_E       65537

typedef struct cosm_BN
{
  COSM_BN_WORD  * n;
  COSM_BN_WORD  length;
  COSM_BN_SWORD digits;
  u32 neg;
} cosm_BN;

s32 CosmBNInit( cosm_BN * x );
  /*
    Initialize the bignum to 0. This isn't neccesary if you have allocated
    the cosm_BN with CosmMemAlloc. The library will handle numbers up to
    2^31 - 64 bits in length or the limit of memory.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNSet( cosm_BN * x, const cosm_BN * a );
  /*
    x = a.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNSets32( cosm_BN * x, const s32 a );
  /*
    x = (s32) a.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNGets32( s32 * x, const cosm_BN * a );
  /*
    x = (s32) a. Nasty truncation will occur if the number is more then
    31 bits long.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNCmp( const cosm_BN * a, const cosm_BN * b );
  /*
    Compare the numbers a and b.
    Returns: -1 if a < b, 0 if a == b, 1 if a > b.
  */

s32 CosmBNOdd( const cosm_BN * x );
  /*
    Is x odd?
    Returns: 1 if x is odd, 0 if even.
  */

s32 CosmBNZero( const cosm_BN * x );
  /*
    Is x zero?
    Returns: 1 if x is zero, 0 otherwise.
  */

s32 CosmBNBits( u32 * x, const cosm_BN * a );
  /*
    x = bits in a, could be 0.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNAdd( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = a + b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNSub( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = a - b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNMul( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = a * b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNDiv( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = a / b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNMod( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = a % b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNModExp( cosm_BN * x, const cosm_BN * a, const cosm_BN * e,
  const cosm_BN * m );
  /*
    x = a to the e-th power modulo m.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNModInv( cosm_BN * x, const cosm_BN * a, const cosm_BN * m );
  /*
    x = Inverse of a modulo m.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNGCD( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    x = Greatest Common Divisor (GCD) of a and b. 1 if a and b are
    relatively prime.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNPrimeRM( const cosm_BN * p, u32 tests,
  void (*callback)( s32, s32, void * ), void * param );
  /*
    Run tests tests to check if p is prime with Rabin-Miller.
    If tests < 5, enough tests will be run to give a possible error
    of 2^-80 for small (<512bit) numbers. At least 5 tests will be run
    which is enough for large (>512bit) primes. No shortage of Rabin-Miller
    documentation exists, read it before using this function with tests != 0.
    Returns: 1 if p is probably prime, 0 if not prime.
  */

s32 CosmBNPrimeGen( cosm_BN * x, u32 bits, const u8 * rnd_bits,
  void (*callback)( s32, s32, void * ), void * param );
  /*
    Generate a large, probably prime number, x = bits bit prime.
    bits must be a multiple of 8 and at least 256. The rnd_bits must be at
    least bits long. The rnd_bits should be as random as possible, as
    using psuedo-random bits will get your keys cracked. x-1 will always
    be relatively prime to COSM_BN_E, so x is safe to use for RSA.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNStr( cosm_BN * x, void ** end, const void * string, u32 radix );
  /*
    Convert the string written in radix to the bignum x.
    Numbers are of the form: [space*][+|-][0|0x|0X]{0-9a-zA-Z}+
    radix must be 2-36 or 0. If radix is 0, numbers starting with 0x|0X will
    be read as base 16, numbers starting with 0 will be interpreted as base 8,
    and all others will be base 10. If end is not NULL, it will be set to the
    character after the last character used in the number.
    Note that use of radixes other then 2, 8, 10, or 16 are generally useless.
    Returns: sets result to number and returns COSM_PASS on success, or sets
      result to 0 and returns COSM_FAIL on failure.
  */

u32 CosmBNPrint( cosm_FILE * file, utf8 * string, u32 max_bytes,
  u32 radix, cosm_BN * x );
  /*
    Prints x to the string in the radix. No more than max_bytes-1
    characters will be written to the string. radix must be from 2 and 16.
    Returns: Number of characters written to string, or -1 if truncated due
      to max_bytes.
  */

void CosmBNFree( cosm_BN * x );
  /*
    Free the bignum internals.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNLoad( cosm_BN * x, const u8 * bytes, u32 bits );
  /*
    Load bits bits of big endian bytes into x. bits must be a multiple of 8.
    The largest number you can load is 2^31 - 64 bits long (COSM_BN_MAXBITS).
    Do not load more bits the number originally had when saved.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 CosmBNSave( u8 * bytes, const cosm_BN * x, u32 max_bits,
  u32 min_bits );
  /*
    Save |x| into bytes in big endian order. If the number is more then
    max_bits long the function will fail and no bytes will be saved.
    If the number is less then min_bits (also a multiple of 8) the high
    bits will be padded.
    The largest number you can save is 2^31 - 64 bits long (COSM_BN_MAXBITS).
    Care should be taken to record how many bits were in the number for
    later loading.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

/* low level */

s32 Cosm_BNGrow( cosm_BN * x, const COSM_BN_WORD length );
  /*
    Expand x to be at least length words long.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNExplode( cosm_BN * x );
  /*
    Expand x into twice as many half-words.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

void Cosm_BNImplode( cosm_BN * x );
  /*
    Restore x back to full words.
    Returns: Nothing.
  */

s32 Cosm_BNuAdd( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    Implementation of unsigned x = a + b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNuSub( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    Implementation of unsigned x = a - b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNFastSub( cosm_BN * x, const cosm_BN * a );
  /*
    Implementation of x -= a, where a < x.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNuMul( cosm_BN * x, const cosm_BN * a, const cosm_BN * b );
  /*
    Implementation of unsigned x = a * b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNDivMod( cosm_BN * x, cosm_BN * y,
  const cosm_BN * a, const cosm_BN * b );
  /*
    Implementation of x = a / b, y = a % b.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNuCmp( const cosm_BN * a, const cosm_BN * b );
  /*
    Compare the numbers |a| and |b|.
    Returns: -1 if a < b, 0 if a == b, 1 if a > b.
  */

s32 Cosm_BNLsh( cosm_BN * x, const cosm_BN * a, const u32 s );
  /*
    x = a << s.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNRsh1( cosm_BN * x );
  /*
    x = x >> 1.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

s32 Cosm_BNBitSet( cosm_BN * x, u32 b, u32 value );
  /*
    Set's the b'th bit (0 based) of x to value. Before calling this function
    make sure x is already Cosm_BNGrow'n large enough.
    Returns: COSM_PASS or COSM_FAIL on an error.
  */

/* testing */

s32 Cosm_TestBigNum( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
