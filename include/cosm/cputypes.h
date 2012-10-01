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
/**
\file cputypes.h
\brief Cosm CPU/OS defines, and scalar/vector data types.
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
#define OS_WIN64        2  /* 64bit XP/2003+/Vista */
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
#define COSM_OS_TYPE_MAX  17

#define COSM_OS_TYPES { "UNKNOWN", \
  "Win32", "Win64", "OSX", "Linux", "undef", \
  "NetBSD", "FreeBSD", "OpenBSD", "undef", "undef", \
  "Irix", "Irix64", "SunOS", "Solaris", "SonyPS3", \
  "QNX", "Tru64", NULL }

/* OS_TYPE must be defined by now */
#if ( !defined( OS_TYPE ) || ( OS_TYPE == OS_UNKNOWN ) \
  || ( OS_TYPE > COSM_OS_TYPE_MAX ) )
#error "Define OS_TYPE - see cputypes.h"
#endif

/* Endian settings for big endian or little endian */
#undef  COSM_ENDIAN_CURRENT
#define COSM_ENDIAN_BIG     4321
#define COSM_ENDIAN_LITTLE  1234

#if ( ( CPU_TYPE == CPU_X86 ) || ( CPU_TYPE == CPU_X64 ) \
  || ( CPU_TYPE == CPU_IA64 ) || ( CPU_TYPE == CPU_ALPHA ) \
  || ( CPU_TYPE == CPU_ARM ) )
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
typedef unsigned char u8; /**< 8-bit unsigned. */
typedef signed char s8;   /**< 8-bit signed. */
#if ( UCHAR_MAX != 0xFF )
#error "char is not 8 bits - serious problem - see cputypes.h"
#endif

/* Define 16bit types */
typedef unsigned short u16; /**< 16-bit unsigned. */
typedef signed short s16;   /**< 16-bit signed. */
#if ( USHRT_MAX != 0xFFFF )
#error "Cannot find a 16-bit type - see cputypes.h"
#endif

/* Define the 32bit types, int most common */
/**
\typedef u32
32-bit unsigned.
\typedef s32
32-bit signed.
*/
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
/**
\typedef u64
64-bit unsigned.
\typedef s64
64-bit signed.
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
typedef struct u128 { u64 hi; u64 lo; } u128; /**< 128-bit unsigned. */
typedef struct s128 { s64 hi; u64 lo; } s128; /**< 128-bit signed. */

/*
  Define the 128 bit setting and equality macros
  usage: _COSM_SET128( a, fedcba9876543210, 0123456789abcdef );
  usage: if ( _COSM_EQ128( a, fedcba9876543210, 0123456789abcdef ) ) ...
*/
/** @cond SKIPED */
#define _COSM_CAT64( a ) 0x##a##LL
/** @endcond */
#define _COSM_SET128( dest, a, b ) \
  dest.hi = _COSM_CAT64( a ); dest.lo = _COSM_CAT64( b );
#define _COSM_EQ128( value, a, b ) \
  ( ( value.hi == _COSM_CAT64( a ) ) && ( value.lo == _COSM_CAT64( b ) ) )

/* Floating point, not everything has a floating point unit */
#if ( !defined( NO_FLOATING_POINT ) )
typedef float f32;        /**< 32-bit float. */
typedef double f64;       /**< 64 bit double. */
typedef long double f128; /**< 128-bit float. */
#endif

/* Text types */
typedef char ascii;   /**< ASCII 7-bit text. */
typedef char utf8;    /**< String of multibyte UTF-8 Unicode. */
typedef u32 utf8char; /**< Single Unicode codepoint. */

/**
  Cosm time type. A fixed point 128bit signed number. 64 bit seconds and
  64 bits of fraction. Zero is January 1, 2000 at 0:00:00.
*/
typedef s128 cosmtime;

/* Packed structure macros */
/**
\def PACKED_STRUCT_BEGIN
\brief Begin packed memory struct definition.
\def PACKED_STRUCT_END
\brief End packed memory struct definition.
*/
#if defined( __GNUC__ )
#define PACKED_STRUCT_BEGIN _Pragma( "pack(1)" )
#define PACKED_STRUCT_END _Pragma( "pack()" )
#elif defined( _MSC_VER )
#define PACKED_STRUCT_BEGIN __pragma( pack(push,1) );
#define PACKED_STRUCT_END __pragma( pack(pop) );
#else
#error "need correct struct packing macro, see cputypes.h"
#endif

/* Vector types, up to 512 bits */
PACKED_STRUCT_BEGIN
typedef f32 f32x2[2];   /**< f32 vector type. */
typedef f32 f32x4[4];   /**< f32 vector type. */
typedef f32 f32x8[8];   /**< f32 vector type. */
typedef f32 f32x16[16]; /**< f32 vector type. */
typedef f64 f64x2[2];   /**< f64 vector type. */
typedef f64 f64x4[4];   /**< f64 vector type. */
typedef f64 f64x8[8];   /**< f64 vector type. */

typedef s8  s8x2[2];    /**< s8 vector type. */
typedef s8  s8x4[4];    /**< s8 vector type. */
typedef s8  s8x8[8];    /**< s8 vector type. */
typedef s8  s8x16[16];  /**< s8 vector type. */
typedef s8  s8x32[32];  /**< s8 vector type. */
typedef s8  s8x64[64];  /**< s8 vector type. */

typedef s16 s16x2[2];
typedef s16 s16x4[4];
typedef s16 s16x8[8];
typedef s16 s16x16[16];
typedef s16 s16x32[32];

typedef s32 s32x2[2];
typedef s32 s32x4[4];
typedef s32 s32x8[8];
typedef s32 s32x16[16];

typedef s64 s64x2[2];
typedef s64 s64x4[4];
typedef s64 s64x8[8];

typedef u8  u8x2[2];
typedef u8  u8x4[4];
typedef u8  u8x8[8];
typedef u8  u8x16[16];
typedef u8  u8x32[32];
typedef u8  u8x64[64];

typedef u16 u16x2[2];
typedef u16 u16x4[4];
typedef u16 u16x8[8];
typedef u16 u16x16[16];
typedef u16 u16x32[32];

typedef u32 u32x2[2];
typedef u32 u32x4[4];
typedef u32 u32x8[8];
typedef u32 u32x16[16];

typedef u64 u64x2[2];
typedef u64 u64x4[4];
typedef u64 u64x8[8];
PACKED_STRUCT_END

enum vector_type
{
  /* 3 bytes = type (float, signed, unsigned), bytes, vector width */
  VECTOR_F32    = 0x010401,
  VECTOR_F32X2  = 0x010402,
  VECTOR_F32X4  = 0x010404,
  VECTOR_F32X8  = 0x010408,
  VECTOR_F32X16 = 0x010410,

  VECTOR_F64    = 0x010801,
  VECTOR_F64X2  = 0x010802,
  VECTOR_F64X4  = 0x010804,
  VECTOR_F64X8  = 0x010808,

  VECTOR_S8     = 0x020101,
  VECTOR_S8X2   = 0x020102,
  VECTOR_S8X4   = 0x020104,
  VECTOR_S8X8   = 0x020108,
  VECTOR_S8X16  = 0x020110,
  VECTOR_S8X32  = 0x020120,
  VECTOR_S8X64  = 0x020140,

  VECTOR_S16    = 0x020201,
  VECTOR_S16X2  = 0x020202,
  VECTOR_S16X4  = 0x020204,
  VECTOR_S16X8  = 0x020208,
  VECTOR_S16X16 = 0x020210,
  VECTOR_S16X32 = 0x020220,

  VECTOR_S32    = 0x020401,
  VECTOR_S32X2  = 0x020402,
  VECTOR_S32X4  = 0x020404,
  VECTOR_S32X8  = 0x020408,
  VECTOR_S32X16 = 0x020410,

  VECTOR_S64    = 0x020801,
  VECTOR_S64X2  = 0x020802,
  VECTOR_S64X4  = 0x020804,
  VECTOR_S64X8  = 0x020808,

  VECTOR_U8     = 0x030101,
  VECTOR_U8X2   = 0x030102,
  VECTOR_U8X4   = 0x030104,
  VECTOR_U8X8   = 0x030108,
  VECTOR_U8X16  = 0x030110,
  VECTOR_U8X32  = 0x030120,
  VECTOR_U8X64  = 0x030140,

  VECTOR_U16    = 0x030201,
  VECTOR_U16X2  = 0x030202,
  VECTOR_U16X4  = 0x030204,
  VECTOR_U16X8  = 0x030208,
  VECTOR_U16X16 = 0x030210,
  VECTOR_U16X32 = 0x030220,

  VECTOR_U32    = 0x030401,
  VECTOR_U32X2  = 0x030402,
  VECTOR_U32X4  = 0x030404,
  VECTOR_U32X8  = 0x030408,
  VECTOR_U32X16 = 0x030410,

  VECTOR_U64    = 0x030801,
  VECTOR_U64X2  = 0x030802,
  VECTOR_U64X4  = 0x030804,
  VECTOR_U64X8  = 0x030808
};

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
#define EXPORT
#endif

/* universal pass/fail */
#define COSM_PASS  0
#define COSM_FAIL -1

#endif
