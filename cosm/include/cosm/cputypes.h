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

/**
  @file cputypes.h
  Cosm CPU/OS defines, scalar types, and vector data types.
*/

/* CPU/OS Layer - CPU and OS specific code is allowed */

#ifndef COSM_CPUTYPES_H
#define COSM_CPUTYPES_H

#include <limits.h>

/* CPU family. */
#define CPU_INVALID   0  /** Invalid CPU */
#define CPU_X86       1  /** Intel 8086 */
#define CPU_X64       2  /** AMD64 */
#define CPU_ARM       3  /** ARM */
#define CPU_ARM64     4  /** ARM64 */
#define CPU_MIPS      5  /** MIPS */
#define CPU_MIPS64    6  /** MIPS64 */
#define CPU_PPC       7  /** PowerPC */
#define CPU_PPC64     8  /** PowerPC 64 */
#define CPU_TYPE_MAX  8  /** Maximum valid CPU_TYPE */

#define COSM_CPU_TYPES { "INVALID", \
  "x86", "x64", "ARM", "ARM64", "MIPS", "MIPS64", "PPC", "PPC64", \
  NULL }

#if ( !defined( CPU_TYPE ) )
#  if ( defined( __x86_64__ ) || defined( _M_X64 ) )
#    define CPU_TYPE CPU_X64
#  elif ( defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) \
     || defined( __i686 ) || defined( _M_IX86 ) )
#    define CPU_TYPE CPU_X86
#  elif ( defined( __aarch64__ ) )
#    define CPU_TYPE CPU_ARM64
#  elif ( defined( __arm__ ) || defined( _M_ARM ) )
#    define CPU_TYPE CPU_ARM
#  elif ( defined( __mips64__ ) )
#    define CPU_TYPE CPU_MIPS64
#  elif ( defined( __mips__ ) )
#    define CPU_TYPE CPU_MIPS
#  elif ( defined( __ppc__ ) || defined( _M_PPC ) )
#    defined CPU_TYPE CPU_PPC
#  elif ( defined( __ppc64__ ) )
#    defined CPU_TYPE CPU_PPC64
#  endif
#endif

/* CPU_TYPE must be valid */
#if ( !defined( CPU_TYPE ) || ( CPU_TYPE <= CPU_INVALID ) \
  || ( CPU_TYPE > CPU_TYPE_MAX ) )
#  error "Improperly defined or undetected CPU_TYPE - see cputypes.h"
#endif

#define OS_INVALID    0  /** Invalid OS */
#define OS_WIN32      1  /** Win32 - 32bit WinNT 4, Win2000, XP and up */
#define OS_WIN64      2  /** Win64 - 64bit XP, 2003+, Vista and up */
#define OS_LINUX      3  /** Linux */
#define OS_ANDROID    4  /** Android */
#define OS_OSX        5  /** Apple OS X */
#define OS_IOS        6  /** Apple iOS */
#define OS_SOLARIS    7  /** Solaris */
#define OS_FREEBSD    8  /** FreeBSD */
#define OS_OPENBSD    9  /** OpenBSD */
#define OS_NETBSD     10 /** NetBSD */
#define OS_TYPE_MAX   10

#define COSM_OS_TYPES { "INVALID", \
  "Win32", "Win64", "Linux", "Android", "OSX", "iOS", "Solaris", \
  "FreeBSD", "OpenBSD", "NetBSD", NULL }

#if ( !defined( OS_TYPE ) )
#  if ( defined( _WIN64 ) )
#    define OS_TYPE OS_WIN64
#  elif ( defined( _WIN32 ) )
#    define OS_TYPE OS_WIN32
#  elif ( defined( __ANDROID__ ) )
#    define OS_TYPE OS_ANDROID
#  elif ( defined( __linux__ ) )
#    define OS_TYPE OS_LINUX
#  elif ( defined( __APPLE__ ) )
#    include "TargetConditionals.h"
#    if TARGET_OS_IPHONE
#      define OS_TYPE OS_IOS
#    else
#      define OS_TYPE OS_OSX
#    endif
#  endif
#endif

/* OS_TYPE must be valid */
#if ( !defined( OS_TYPE ) || ( OS_TYPE <= OS_INVALID ) \
  || ( OS_TYPE > OS_TYPE_MAX ) )
#  error "Improperly defined or undetected OS_TYPE - see cputypes.h"
#endif

/* Endian settings for big endian or little endian */
#define COSM_ENDIAN_BIG     4321
#define COSM_ENDIAN_LITTLE  1234

#if ( !defined( COSM_ENDIAN ) )
#  if ( ( CPU_TYPE == CPU_X86 ) || ( CPU_TYPE == CPU_X64 ) \
     || ( CPU_TYPE == CPU_ARM ) || ( CPU_TYPE == CPU_ARM64 ) \
     || ( CPU_TYPE == CPU_MIPS ) || ( CPU_TYPE == CPU_ARM64 ) \
     || ( CPU_TYPE == CPU_PPC ) || ( CPU_TYPE == CPU_PPC64 ) )
#    define COSM_ENDIAN COSM_ENDIAN_LITTLE
#  else
#    define COSM_ENDIAN COSM_ENDIAN_BIG
#  endif
#endif

/* COSM_ENDIAN must be valid */
#if ( !defined( COSM_ENDIAN ) || ( ( COSM_ENDIAN != COSM_ENDIAN_LITTLE ) \
  && ( COSM_ENDIAN != COSM_ENDIAN_BIG ) ) )
#  error "A COSM_ENDIAN must be specified - see cputypes.h"
#endif


#if ( ( CPU_TYPE == CPU_X86 ) || ( CPU_TYPE == CPU_ARM ) \
  || ( CPU_TYPE == CPU_MIPS ) || ( CPU_TYPE == CPU_PPC ) )
#  define CPU_32BIT
#  undef CPU_64BIT
#elif ( ( CPU_TYPE == CPU_X64 ) || ( CPU_TYPE == CPU_ARM64 ) \
  || ( CPU_TYPE == CPU_MIPS64 ) || ( CPU_TYPE == CPU_PPC64 ) )
#  define CPU_64BIT
#  undef CPU_32BIT
#else
#  error "CPU_TYPE missing CPU_64BIT/CPU_32BIT - see cputypes.h"
#endif

/** 8-bit unsigned integer. */
typedef unsigned char u8;
/** 8-bit signed integer. */
typedef signed char s8;
#if ( UCHAR_MAX != 0xFF )
#  error "char is not 8 bits - serious problem - see cputypes.h"
#endif

/** 16-bit unsigned integer. */
typedef unsigned short u16;
/** 16-bit signed integer. */
typedef signed short s16;
#if ( USHRT_MAX != 0xFFFF )
#  error "Cannot find a 16-bit type - see cputypes.h"
#endif

#if ( UINT_MAX == 0xFFFFFFFF )
/** 32-bit unsigned integer. */
typedef unsigned int u32;
/** 32-bit signed integer. */
typedef signed int s32;
#elif ( ULONG_MAX == 0xFFFFFFFF )
typedef unsigned long u32;
typedef signed long s32;
#else
#  error "Cannot find a 32-bit type - see cputypes.h"
#endif

/* On a CPU_32BIT this type is being implemented by the compiler. */
#if ( defined( CPU_64BIT ) )
/** 64-bit unsigned integer. */
typedef unsigned long long u64;
/** 64-bit signed integer. */
typedef signed long long s64;
#else /* 32 bit CPU */
#  if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
typedef unsigned __int64 u64;
typedef signed __int64 s64;
#  else /* not win32 */
typedef unsigned long long u64;
typedef signed long long s64;
#  endif /* OS */
#endif /* 64 or 32 bit ? */

/* All systems have software 128bit integers */

/** 128-bit unsigned integer. */
typedef struct u128
{
  u64 hi; /**< High order 64 bits. */
  u64 lo; /**< Low order 64 bits. */
} u128;

/** 128-bit signed integer. */
typedef struct s128
{
  s64 hi; /**< High order 64 bits. */
  u64 lo; /**< Low order 64 bits. */
} s128;

/**
  Helper Macro for _COSM_SET128 and _COSM_EQ128.
  @see _COSM_SET128
  @see _COSM_EQ128
*/
#define _COSM_CAT64( a ) 0x##a##LL

/**
  Set a 128-bit value from 2 64-bit numbers.
  @param dest Destination.
  @param a High order 64 bits.
  @param b Low order 64 bits.
  @code
    _COSM_SET128( a, 0123456789ABCDEF, FEDCBA9876543210 );
  @endcode
  @see _COSM_CAT64
*/
#define _COSM_SET128( dest, a, b ) \
  dest.hi = _COSM_CAT64( a ); dest.lo = _COSM_CAT64( b );

/**
  Compare a 128-bit value by parts to 64-bit numbers.
  @param value Value to compare to.
  @param a High order 64 bits.
  @param b Low order 64 bits.
  @code
    if ( !_COSM_EQ128( foo, 123456789ABCDEF4, 5670123CDEF89AB0 ) )
    {
      return -1;
    }
  @endcode
  @see _COSM_CAT64
*/
#define _COSM_EQ128( value, a, b ) \
  ( ( value.hi == _COSM_CAT64( a ) ) && ( value.lo == _COSM_CAT64( b ) ) )

/* Not every CPU has a floating point unit. */
#if ( !defined( NO_FLOATING_POINT ) )
/** 32-bit floating point value. */
typedef float f32;
/** 64-bit floating point value. */
typedef double f64;
#endif

/** String of ASCII 7-bit text. */
typedef char ascii;
/** String of multibyte UTF-8 Unicode. */
typedef char utf8;
/** Single Unicode character codepoint. */
typedef u32 utf8char;

/**
  Cosm internal time type. A fixed point 128bit signed number,
  with 64 bit seconds and 64 bits of fractional seconds.
  Zero was January 1, 2000 at 00:00:00.
*/
typedef s128 cosmtime;

/* Packed structure macros */
/**
@def PACKED_STRUCT_BEGIN
Begin packed memory structure definition.
@def PACKED_STRUCT_END
End packed structure definitions.
*/
#if defined( __GNUC__ )
#  define PACKED_STRUCT_BEGIN _Pragma( "pack(push,1)" )
#  define PACKED_STRUCT_END _Pragma( "pack(pop)" )
#elif defined( _MSC_VER )
#  define PACKED_STRUCT_BEGIN __pragma( pack(push,1) )
#  define PACKED_STRUCT_END __pragma( pack(pop) )
#else
#  error "need correct struct packing macro, see cputypes.h"
#endif

/* Vector types, up to 512 bits */

PACKED_STRUCT_BEGIN
typedef f32 f32x2[2];   /**< f32 x2 vector type. */
typedef f32 f32x4[4];   /**< f32 x4 vector type. */
typedef f32 f32x8[8];   /**< f32 x8 vector type. */
typedef f32 f32x16[16]; /**< f32 x16 vector type. */
typedef f64 f64x2[2];   /**< f64 x2 vector type. */
typedef f64 f64x4[4];   /**< f64 x4 vector type. */
typedef f64 f64x8[8];   /**< f64 x8 vector type. */

typedef s8  s8x2[2];    /**< s8 x2 vector type. */
typedef s8  s8x4[4];    /**< s8 x4 vector type. */
typedef s8  s8x8[8];    /**< s8 x8 vector type. */
typedef s8  s8x16[16];  /**< s8 x16 vector type. */
typedef s8  s8x32[32];  /**< s8 x32 vector type. */
typedef s8  s8x64[64];  /**< s8 x64 vector type. */

typedef s16 s16x2[2];   /**< s16 x2 vector type. */
typedef s16 s16x4[4];   /**< s16 x4 vector type. */
typedef s16 s16x8[8];   /**< s16 x8 vector type. */
typedef s16 s16x16[16]; /**< s16 x16 vector type. */
typedef s16 s16x32[32]; /**< s16 x32 vector type. */

typedef s32 s32x2[2];   /**< s32 x2 vector type. */
typedef s32 s32x4[4];   /**< s32 x4 vector type. */
typedef s32 s32x8[8];   /**< s32 x8 vector type. */
typedef s32 s32x16[16]; /**< s32 x16 vector type. */

typedef s64 s64x2[2];   /**< s64 x2 vector type. */
typedef s64 s64x4[4];   /**< s64 x4 vector type. */
typedef s64 s64x8[8];   /**< s64 x8 vector type. */

typedef u8  u8x2[2];    /**< u8 x2 vector type. */
typedef u8  u8x4[4];    /**< u8 x4 vector type. */
typedef u8  u8x8[8];    /**< u8 x8 vector type. */
typedef u8  u8x16[16];  /**< u8 x16 vector type. */
typedef u8  u8x32[32];  /**< u8 x32 vector type. */
typedef u8  u8x64[64];  /**< u8 x64 vector type. */

typedef u16 u16x2[2];   /**< u16 x2 vector type. */
typedef u16 u16x4[4];   /**< u16 x4 vector type. */
typedef u16 u16x8[8];   /**< u16 x8 vector type. */
typedef u16 u16x16[16]; /**< u16 x16 vector type. */
typedef u16 u16x32[32]; /**< u16 x32 vector type. */

typedef u32 u32x2[2];   /**< u32 x2 vector type. */
typedef u32 u32x4[4];   /**< u32 x4 vector type. */
typedef u32 u32x8[8];   /**< u32 x8 vector type. */
typedef u32 u32x16[16]; /**< u32 x16 vector type. */

typedef u64 u64x2[2];   /**< u64 x2 vector type. */
typedef u64 u64x4[4];   /**< u64 x4 vector type. */
typedef u64 u64x8[8];   /**< u64 x8 vector type. */
PACKED_STRUCT_END

/**
  Enums for referring to Cosm vector types.
  Value is: ( [float=1, signed=2, unsigned=3] << 16 ) +
  ( base_type_bytes << 8 ) + vector width.
*/
enum vector_type
{
  VECTOR_F32    = 0x010401, /**< f32 */
  VECTOR_F32X2  = 0x010402, /**< f32x2 */
  VECTOR_F32X4  = 0x010404, /**< f32x4 */
  VECTOR_F32X8  = 0x010408, /**< f32x8 */
  VECTOR_F32X16 = 0x010410, /**< f32x16 */

  VECTOR_F64    = 0x010801, /**< f64 */
  VECTOR_F64X2  = 0x010802, /**< f64x2 */
  VECTOR_F64X4  = 0x010804, /**< f64x4 */
  VECTOR_F64X8  = 0x010808, /**< f64x8 */

  VECTOR_S8     = 0x020101, /**< s8 */
  VECTOR_S8X2   = 0x020102, /**< s8x2 */
  VECTOR_S8X4   = 0x020104, /**< s8x4 */
  VECTOR_S8X8   = 0x020108, /**< s8x8 */
  VECTOR_S8X16  = 0x020110, /**< s8x16 */
  VECTOR_S8X32  = 0x020120, /**< s8x32 */
  VECTOR_S8X64  = 0x020140, /**< s8x64 */

  VECTOR_S16    = 0x020201, /**< s16 */
  VECTOR_S16X2  = 0x020202, /**< s16x2 */
  VECTOR_S16X4  = 0x020204, /**< s16x4 */
  VECTOR_S16X8  = 0x020208, /**< s16x8 */
  VECTOR_S16X16 = 0x020210, /**< s16x16 */
  VECTOR_S16X32 = 0x020220, /**< s16x32 */

  VECTOR_S32    = 0x020401, /**< s32 */
  VECTOR_S32X2  = 0x020402, /**< s32x2 */
  VECTOR_S32X4  = 0x020404, /**< s32x4 */
  VECTOR_S32X8  = 0x020408, /**< s32x8 */
  VECTOR_S32X16 = 0x020410, /**< s32x16 */

  VECTOR_S64    = 0x020801, /**< s64 */
  VECTOR_S64X2  = 0x020802, /**< s64x2 */
  VECTOR_S64X4  = 0x020804, /**< s64x4 */
  VECTOR_S64X8  = 0x020808, /**< s64x8 */

  VECTOR_U8     = 0x030101, /**< u8 */
  VECTOR_U8X2   = 0x030102, /**< u8x2 */
  VECTOR_U8X4   = 0x030104, /**< u8x4 */
  VECTOR_U8X8   = 0x030108, /**< u8x8 */
  VECTOR_U8X16  = 0x030110, /**< u8x16 */
  VECTOR_U8X32  = 0x030120, /**< u8x32 */
  VECTOR_U8X64  = 0x030140, /**< u8x64 */

  VECTOR_U16    = 0x030201, /**< u16 */
  VECTOR_U16X2  = 0x030202, /**< u16x2 */
  VECTOR_U16X4  = 0x030204, /**< u16x4 */
  VECTOR_U16X8  = 0x030208, /**< u16x8 */
  VECTOR_U16X16 = 0x030210, /**< u16x16 */
  VECTOR_U16X32 = 0x030220, /**< u16x32 */

  VECTOR_U32    = 0x030401, /**< u32 */
  VECTOR_U32X2  = 0x030402, /**< u32x2 */
  VECTOR_U32X4  = 0x030404, /**< u32x4 */
  VECTOR_U32X8  = 0x030408, /**< u32x8 */
  VECTOR_U32X16 = 0x030410, /**< u32x16 */

  VECTOR_U64    = 0x030801, /**< u64 */
  VECTOR_U64X2  = 0x030802, /**< u64x2 */
  VECTOR_U64X4  = 0x030804, /**< u64x4 */
  VECTOR_U64X8  = 0x030808  /**< u64x8 */
};

/**
  Macro to calculate the number of bytes in a vector_type.
  @param type vector_type to determine the byte length of.
  @see vector_type.
*/
#define _VECTOR_TYPE_BYTES( type ) \
  ( ( ( type >> 8 ) & 0xFF ) * ( type & 0xFF ) )

/* Make sure that NULL defined to standard. */
#undef  NULL
#ifdef __cplusplus
#  define NULL 0
#else
#  define NULL ((void *) 0)
#endif

/* Dynamic Library export */
#if defined( USE_EXPORTS ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#  define EXPORT __declspec( dllexport )
#else
#  define EXPORT
#endif

/* universal pass/fail */
#define COSM_PASS  0
#define COSM_FAIL -1

#endif
