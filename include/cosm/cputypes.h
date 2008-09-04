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

#ifndef COSM_CPUTYPES_H
#define COSM_CPUTYPES_H

#include <limits.h>

/* We don't want or need fine resolution on either OS or CPU for compiles */
#define CPU_UNKNOWN     0
#define CPU_X86         1
#define CPU_PPC         2
#define CPU_MIPS        3
#define CPU_MIPS64      4
#define CPU_ALPHA       5
/*#define CPU_PA_RISC   6  depreciated */
#define CPU_68K         7
#define CPU_SPARC       8
#define CPU_SPARC64     9
/*#define CPU_POWER     10 depreciated */
#define CPU_IA64        11
#define CPU_ARM         12
#define CPU_CELL        13
/*#define CPU_S390      14 depreciated */
#define CPU_PPC64       15
#define CPU_X64         16
#define COSM_CPU_TYPE_MAX 16

#define COSM_CPU_TYPES { "UNKNOWN", \
  "x86", "PPC", "MIPS", "MIPS64", "Alpha", \
  "undef", "68K", "Sparc", "Sparc64", "undef", \
  "IA64",  "ARM", "Cell", "undef", "PPC64", \
  "x64", NULL }

/* CPU_TYPE must be defined by now */
#if ( !defined( CPU_TYPE ) || ( CPU_TYPE == CPU_UNKNOWN ) \
  || ( CPU_TYPE > COSM_CPU_TYPE_MAX ) )
#error "Define CPU_TYPE - see cputypes.h"
#endif

#define OS_UNKNOWN      0
#define OS_WIN32        1  /* 32bit WinNT 4, Win2000, XP and up */
/*#define OS_MACOS      2  depreciated, never completed */
#define OS_MACOSX       3
#define OS_LINUX        4
/*#define OS_BSDI       5  depreciated */
#define OS_NETBSD       6
#define OS_FREEBSD      7
#define OS_OPENBSD      8
/*#define OS_NEXTSTEP   9  depreciated */
/*#define OS_BEOS       10 depreciated */
#define OS_IRIX         11
#define OS_IRIX64       12
#define OS_SUNOS        13
#define OS_SOLARIS      14
#define OS_SONYPS3      15 /* !!!this must have a name */
#define OS_QNX          16
#define OS_TRU64        17 /* Dec UNIX, OSF1 */
#define OS_WIN64        18 /* 64bit XP/2003+/Vista */
#define COSM_OS_TYPE_MAX  18

#define COSM_OS_TYPES { "UNKNOWN", \
  "Win32", "undef", "MacOSX", "Linux", "undef", \
  "NetBSD", "FreeBSD", "OpenBSD", "undef", "undef", \
  "Irix", "Irix64", "SunOS", "Solaris", "SonyPS3", \
  "QNX", "Tru64", "Win64", NULL }

/* OS_TYPE must be defined by now */
#if ( !defined( OS_TYPE ) || ( OS_TYPE == OS_UNKNOWN ) \
  || ( OS_TYPE > COSM_OS_TYPE_MAX ) )
#error "Define OS_TYPE - see cputypes.h"
#endif

/* Endian settings for big endian or idiot endian */
#undef  COSM_ENDIAN_CURRENT
#define COSM_ENDIAN_BIG     4321
#define COSM_ENDIAN_LITTLE  1234

#if ( ( CPU_TYPE == CPU_X86 ) || ( CPU_TYPE == CPU_X64 ) \
  || ( CPU_TYPE == CPU_IA64 ) || ( CPU_TYPE == CPU_ALPHA ) )
#define COSM_ENDIAN_CURRENT COSM_ENDIAN_LITTLE
#else
#define COSM_ENDIAN_CURRENT COSM_ENDIAN_BIG
#endif

/* One and only one of the following must be defined */
#if ( ( !defined( CPU_64BIT ) && !defined( CPU_32BIT ) ) \
  || ( defined( CPU_64BIT ) && defined( CPU_32BIT ) ) )
#error "Define either CPU_64BIT or CPU_32BIT"
#endif

/* Define 8bit types and characters */
#if ( UCHAR_MAX == 0xFF )
typedef unsigned char u8;
typedef signed char s8;
#else
#error "char is not 8 bits - serious problem - see cputypes.h"
#endif

/* Define 16bit types */
#if ( USHRT_MAX == 0xFFFF )
typedef unsigned short u16;
typedef signed short s16;
#else
#error "Cannot find a 16-bit type - see cputypes.h"
#endif

/* Define the 32bit types, int most common */
#if ( UINT_MAX == 0xFFFFFFFF )
typedef unsigned int u32;
typedef signed int s32;
#elif ( ULONG_MAX == 0xFFFFFFFF )
typedef unsigned long u32;
typedef signed long s32;
#else
#error "Cannot find a 32-bit type - see cputypes.h"
#endif

/*
  Define 64bit types
  On a CPU_32BIT this type is being faked by the compiler.
*/
#if ( defined( CPU_64BIT ) )
#if ( ULONG_MAX == 0xFFFFFFFF )
typedef unsigned long long u64;
typedef signed long long s64;
#else /* long must be 64-bit */
typedef unsigned long u64;
typedef signed long s64;
#endif
#else /* 32 bit CPU */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
typedef unsigned __int64 u64;
typedef signed __int64 s64;
#else /* not win32 */
typedef unsigned long long u64;
typedef signed long long s64;
#endif /* OS */
#endif /* 64 or 32 bit ? */

/* We don't support native 128bit CPUs. All systems will fake 128bit */
typedef struct u128 { u64 hi; u64 lo; } u128;
typedef struct s128 { s64 hi; u64 lo; } s128;

/*
  Define the 128 bit setting and equality macros
  usage: _COSM_SET128( a, fedcba9876543210, 0123456789abcdef );
  usage: if ( _COSM_EQ128( a, fedcba9876543210, 0123456789abcdef ) ) ...
*/
#define _COSM_CAT64( a ) 0x##a##LL
#define _COSM_SET128( dest, a, b ) \
  dest.hi = _COSM_CAT64( a ); dest.lo = _COSM_CAT64( b );
#define _COSM_EQ128( value, a, b ) \
  ( ( value.hi == _COSM_CAT64( a ) ) && ( value.lo == _COSM_CAT64( b ) ) )

/* Floating point, not everything has a floating point unit */
#if ( !defined( NO_FLOATING_POINT ) )
typedef float f32;
typedef double f64;
typedef long double f128;
#endif

/* Text types */
typedef char ascii;   /* ASCII 7-bit text */
typedef char utf8;    /* string of multibyte UTF-8 Unicode */
typedef u32 utf8char; /* single Unicode codepoint */

/* Time type */
typedef s128 cosmtime;

/* Vector types */
typedef struct f32x2 { f32 a, b; } f32x2;
typedef struct f32x3 { f32 a, b, c; } f32x3;
typedef struct f32x4 { f32 a, b, c, d; } f32x4;
typedef struct f64x2 { f64 a, b; } f64x2;
typedef struct f64x3 { f64 a, b, c; } f64x3;
typedef struct f64x4 { f64 a, b, c, d; } f64x4;

typedef struct s8x2  { s8 a, b; } s8x2;
typedef struct s8x3  { s8 a, b, c; } s8x3;
typedef struct s8x4  { s8 a, b, c, d; } s8x4;
typedef struct s16x2 { s16 a, b; } s16x2;
typedef struct s16x3 { s16 a, b, c; } s16x3;
typedef struct s16x4 { s16 a, b, c, d; } s16x4;
typedef struct s32x2 { s32 a, b; } s32x2;
typedef struct s32x3 { s32 a, b, c; } s32x3;
typedef struct s32x4 { s32 a, b, c, d; } s32x4;
typedef struct s64x2 { s64 a, b; } s64x2;
typedef struct s64x3 { s64 a, b, c; } s64x3;
typedef struct s64x4 { s64 a, b, c, d; } s64x4;

typedef struct u8x2  { u8 a, b; } u8x2;
typedef struct u8x3  { u8 a, b, c; } u8x3;
typedef struct u8x4  { u8 a, b, c, d; } u8x4;
typedef struct u16x2 { u16 a, b; } u16x2;
typedef struct u16x3 { u16 a, b, c; } u16x3;
typedef struct u16x4 { u16 a, b, c, d; } u16x4;
typedef struct u32x2 { u32 a, b; } u32x2;
typedef struct u32x3 { u32 a, b, c; } u32x3;
typedef struct u32x4 { u32 a, b, c, d; } u32x4;
typedef struct u64x2 { u64 a, b; } u64x2;
typedef struct u64x3 { u64 a, b, c; } u64x3;
typedef struct u64x4 { u64 a, b, c, d; } u64x4;

typedef enum vector_type
{
  /* 3 bytes = type (float, signed, unsigned), bytes, vector width */
  VECTOR_F32   = 0x010401,
  VECTOR_F32X2 = 0x010402,
  VECTOR_F32X3 = 0x010403,
  VECTOR_F32X4 = 0x010404,
  VECTOR_F64   = 0x010801,
  VECTOR_F64X2 = 0x010802,
  VECTOR_F64X3 = 0x010803,
  VECTOR_F64X4 = 0x010804,
  VECTOR_S8    = 0x020101,
  VECTOR_S8X2  = 0x020102,
  VECTOR_S8X3  = 0x020103,
  VECTOR_S8X4  = 0x020104,
  VECTOR_S16   = 0x020201,
  VECTOR_S16X2 = 0x020202,
  VECTOR_S16X3 = 0x020203,
  VECTOR_S16X4 = 0x020204,
  VECTOR_S32   = 0x020401,
  VECTOR_S32X2 = 0x020402,
  VECTOR_S32X3 = 0x020403,
  VECTOR_S32X4 = 0x020404,
  VECTOR_S64   = 0x020801,
  VECTOR_S64X2 = 0x020802,
  VECTOR_S64X3 = 0x020803,
  VECTOR_S64X4 = 0x020804,
  VECTOR_U8    = 0x030101,
  VECTOR_U8X2  = 0x030102,
  VECTOR_U8X3  = 0x030103,
  VECTOR_U8X4  = 0x030104,
  VECTOR_U16   = 0x030201,
  VECTOR_U16X2 = 0x030202,
  VECTOR_U16X3 = 0x030203,
  VECTOR_U16X4 = 0x030204,
  VECTOR_U32   = 0x030401,
  VECTOR_U32X2 = 0x030402,
  VECTOR_U32X3 = 0x030403,
  VECTOR_U32X4 = 0x030404,
  VECTOR_U64   = 0x030801,
  VECTOR_U64X2 = 0x030802,
  VECTOR_U64X3 = 0x030803,
  VECTOR_U64X4 = 0x030804
} vector_type;

#define _VECTOR_TYPE_BYTES( type ) \
  ( ( ( type >> 8 ) & 0xFF ) * ( type & 0xFF ) )

/* Make sure that NULL is defined and correct */
#undef  NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *) 0)
#endif

/* Dynamic Library export */
#if defined( USE_EXPORTS ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#define EXPORT __declspec( dllexport )
#else
#define EXPORT #undef
#endif

/* universal pass/fail */
#define COSM_PASS  0
#define COSM_FAIL -1

#endif
