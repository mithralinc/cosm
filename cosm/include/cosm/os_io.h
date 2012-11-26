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

#ifndef COSM_OS_IO_H
#define COSM_OS_IO_H

#include "cosm/cputypes.h"
#include "cosm/os_math.h"
#include "cosm/os_file.h"
#include <stdarg.h>

/*
  Standard input and output functions.
*/

#define COSM_IO_ECHO     0
#define COSM_IO_NOECHO   1

#define COSM_IO_NOWAIT   13
#define COSM_IO_WAIT     14

#define COSM_IO_UNSIGNED 0
#define COSM_IO_SIGNED   1

#define COSM_IO_DIGIT_SIGN    -1
#define COSM_IO_DIGIT_INVALID -2
#define COSM_IO_DIGIT_SPACE   -3
#define COSM_IO_DIGIT_DOT     -4
#define COSM_IO_DIGIT_E       14
#define COSM_IO_DIGIT_X       33

/*
  The following are defines/macros for the ANSI escape codes. Keep in mind
  that not all devices are ANSI, so use them only in fitting applications.
  If you're doing large amounts of ANSI, embedding them into strings
  will be slightly faster (if less readable), and this table will be a
  convienient reference.
*/
#define ANSI_CLEAR         "\033[0m"  /* clear all modes */
#define ANSI_HIGH          "\033[1m"  /* high intensity */
#define ANSI_LOW           "\033[2m"  /* low intensity */
#define ANSI_UNDERLINE_ON  "\033[4m"
#define ANSI_UNDERLINE_OFF "\033[24m"
#define ANSI_BLINK_ON      "\033[5m"
#define ANSI_BLINK_OFF     "\033[25m"
#define ANSI_INVERSE_ON    "\033[7m"
#define ANSI_INVERSE_OFF   "\033[27m"
#define ANSI_INVISIBLE     "\033[8m"  /* no display, NOT security */
#define ANSI_SAVE          "\033[s"   /* Saves cursor position */
#define ANSI_RESTORE       "\033[u"   /* Return to saved position */
#define ANSI_CLEARSCREEN   "\033[2J"  /* Clear screen and home */
#define ANSI_CLEARLINE     "\033[K"   /* Clear to end of line */
#define ANSI_FG_BLACK      "\033[30m" /* foreground colors */
#define ANSI_FG_RED        "\033[31m"
#define ANSI_FG_GREEN      "\033[32m"
#define ANSI_FG_YELLOW     "\033[33m"
#define ANSI_FG_BLUE       "\033[34m"
#define ANSI_FG_MAGENTA    "\033[35m"
#define ANSI_FG_CYAN       "\033[36m"
#define ANSI_FG_WHITE      "\033[37m"
#define ANSI_BG_BLACK      "\033[40m" /* background colors */
#define ANSI_BG_RED        "\033[41m"
#define ANSI_BG_GREEN      "\033[42m"
#define ANSI_BG_YELLOW     "\033[43m"
#define ANSI_BG_BLUE       "\033[44m"
#define ANSI_BG_MAGENTA    "\033[45m"
#define ANSI_BG_CYAN       "\033[46m"
#define ANSI_BG_WHITE      "\033[47m"
#define _ANSI_MOVE( line, column ) \
  CosmPrint( "\033[%u;%uH", line, column )
#define _ANSI_UP( count ) \
  CosmPrint( "\033[%uA", count )
#define _ANSI_DOWN( count ) \
  CosmPrint( "\033[%uB", count )
#define _ANSI_FORWARD( count ) \
  CosmPrint( "\033[%uC", count )
#define _ANSI_BACK( count ) \
  CosmPrint( "\033[%uD", count )

u32 CosmInputRaw( utf8 * buffer, u32 length, u32 echo );
  /*
    Read length bytes from stdin into the buffer. If echo is
    COSM_IO_NOECHO then the typed characters will not be echoed in any way,
    otherwise echo should be COSM_IO_ECHO. The buffer will not be terminated
    with a 0, becasue it is not a string. Usually used with a length of 1.
    Returns: Number of bytes read, 0xFFFFFFFF (-1) indicates EOF.
  */

u32 CosmInput( utf8 * buffer, u32 max_bytes, u32 echo );
  /*
    Reads up to max_bytes-1 bytes from standard input device
    until EOF or '\n' is reached. buffer will always be a terminated
    string if input is read. If echo is COSM_IO_NOECHO then the typed
    characters will not be echoed in any way (for password entry etc),
    otherwise echo should be COSM_IO_ECHO.
    Returns: Number of characters input, 0xFFFFFFFF (-1) indicates EOF.
  */

u32 CosmStrBytes( const utf8 * string );
  /*
    Get the length in characters of the UTF-8 string.
    Any incorrectly encoded UTF-8 will result in a short count.
    Returns: the length of the string in characters.
  */

s32 CosmStrCopy( utf8 * dest, const utf8 * src, u32 max_bytes );
  /*
    Copy a string of up to max_length-1 bytes from src to dest,
    including the terminating \0. If the src string is longer then
    max_bytes-1 the function will fail and do nothing.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmStrAppend( utf8 * stringA, const utf8 * stringB, u32 max_bytes );
  /*
    Appends B to the end of A, only if A+B < ( max_bytes - 1 ).
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmStrCmp( const utf8 * stringA, const utf8 * stringB, u32 max_bytes );
  /*
    Compare the 2 strings, up to max_bytes or the end of one string.
    Returns: 0 if the strings are identical, a postive number if the Unicode
      code of the first different character in stringA is greater than the
      Unicode code of that character in stringB, or a negative number if the
      Unicode code of the first different character in stringA is less than
      the Unicode code of that character in stringB.
  */

utf8 * CosmStrChar( const utf8 * string, utf8char character, u32 max_bytes );
  /*
    Locate character in the string, which is up to max_bytes long.
    Returns: pointer to the first occurance of character matching in
      the string, or NULL if it does not occur in the string.
  */

utf8 * CosmStrStr( const utf8 * string, const utf8 * substring,
  u32 max_bytes );
  /*
    Locate substring in the string, which is up to max_bytes-1 long.
    Returns: pointer to the first match of substring in string,
      or NULL if it does not occur in the string.
  */

/*
  The Cosm{type}Str family of functions. Macro's for speed.
*/
#define CosmU32Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 4, COSM_IO_UNSIGNED, radix )
#define CosmS32Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 4, COSM_IO_SIGNED, radix )
#define CosmU64Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 8, COSM_IO_UNSIGNED, radix )
#define CosmS64Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 8, COSM_IO_SIGNED, radix )
#define CosmU128Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 16, COSM_IO_UNSIGNED, radix )
#define CosmS128Str( result, end, string, radix ) \
  Cosm_ParseInt( (void *) result, end, string, 16, COSM_IO_SIGNED, radix )
  /*
    Convert the ascii or Unicode string written in radix to the number type.
    ascii numbers are of the form: [space*][+|-][0|0x|0X]{0-9a-zA-Z}+
    Unicode numbers accept numbers/letters in any language form.
    radix must be 2-36 or 0. If radix is 0, numbers starting with 0x|0X will
    be read as base 16, numbers starting with 0 will be interpreted as base 8,
    and all others will be base 10. If end is not NULL, it will be set to the
    byte after the last byte used by the number.
    For u8 and u16 types use the u32 function and typecast the result.
    Note that use of radixes other then 2, 8, 10, or 16 are generally useless.
    Returns: sets result to number and returns COSM_PASS on success, or sets
      result to 0 and returns COSM_FAIL on failure.
  */

#if ( !defined( NO_FLOATING_POINT ) )
#define Cosmf32Str( result, end, string ) \
  Cosm_ParseFloat( (void *) result, end, string, 4 )
#define Cosmf64Str( result, end, string ) \
  Cosm_ParseFloat( (void *) result, end, string, 8 )
  /*
    Convert the ascii or Unicode base-10 string to a floating point number.
    ascii numbers are of the form:
    [space*][+|-]{{0-9}+[.{0-9}*]|.{0-9}+}[{e|E}[+|-]{0-9}+]
    Unicode numbers accept the base-10 numbers of any language.
    If end is not NULL, it will be set to the byte after the last
    byte used by the number.
    Returns: sets result to the number and returns COSM_PASS on success, or
      sets result to +/-HUGE_VAL if the number was too large or 0 if the
      string wasn't a number and returns COSM_FAIL on failure.
  */
#endif

/*
  Output functions. printf
*/

u32 CosmPrint( const utf8 * format, ... );
  /*
    Prints the formatted text to the standard output device.
    All output buffers will be flushed before return.
    %[width][.prec]type_char
    [width]
      -n = min n characters are printed, left justify
      0n = min n characters are printed, pad 0. No effect type s.
      n = min n characters are printed, right justify.
      *  = next argument (u32) will specify width
    [.prec]
      .n = max n decimals places are printed for type f|F|g|G
           Should always be set, default = 6, capped at 40.
      .n = max n characters printed for type s|b. Default = 0.
           No effect on all other types.
      .* = next argument (u32) will specify precision
    [type_char]
      c = single character
      s = pointer to zero terminated string
      b = pointer to raw byte buffer, precision flag is length in bytes
      f = f32 or f64, [-]dddd.dddddd
      F = f32 or f64, [-]d.dddddd[Edd]
      g = f128, [-]dddd.dddddd
      G = f128, [-]d.dddddd[Edd]
      u = u32 dec
      v = u64 dec
      w = u128 dec
      i = s32 dec
      j = s64 dec
      k = s128 dec
      X = u32 hex (hex = [0-9A-F], always uppercase, no automatic "0x" prefix)
      Y = u64 hex
      Z = u128 hex
      p = pointer zero-padded hex - native length 32bit|64bit
      Note the mnemonics for the 32bit (root) variants:
          c = character, s = string, f = float, b = buffer
          u = unsigned, i = integer, X = hex, p = pointer
    Returns: Number of bytes output.
  */

u32 CosmPrintStr( utf8 * string, u32 max_bytes, const utf8 * format, ... );
  /*
    Prints the formatted text to the string. No more the max_bytes-1
    characters will be written to the string. See CosmPrint for format usage.
    Returns: Number of bytes written to string, or -1 if truncated due
      to max_bytes.
  */

u32 CosmPrintFile( cosm_FILE * file, const utf8 * format, ... );
  /*
    Prints the formatted text to the file.
    See CosmPrint for format usage.
    Returns: Number of bytes written to file.
  */

/* low level */
s32 Cosm_DecodeUTF8( utf8char * codepoint, utf8 ** next, utf8 * stream );
  /*
    Extract a codepoint out of the UTF-8 stream. If next is not NULL,
    it will be set to the byte after the last byte used by the codepoint.
    Returns: COSM_PASS on success, or COSM_FAIL on invalid stream.
  */

s32 Cosm_EncodeUTF8( utf8 * array, u32 * bytes_used, utf8char codepoint );
  /*
    Encode a character into the UTF-8 array. bytes_used is set to the number
    of bytes needed for the encoded character. The array must be at least 6
    bytes long since a codepoint can be up to 6 bytes long.
    Returns: COSM_PASS on success, or COSM_FAIL on invalid codepoint.
  */

s32 Cosm_ParseInt( void * number, utf8 ** end, const utf8 * string,
  u32 number_width, u32 sign, u32 radix );
  /*
    Worker function for Cosm{int}Str functions, places the parsed value into
    number, when number_width is the bytes in the expected value (4, 8, or 16)
    If sign is COSM_IO_SIGNED, then a signed value will be parsed and
    produced, if sign is COSM_IO_UNSIGNED, then an unsigned is parsed and
    producted.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

#if ( !defined( NO_FLOATING_POINT ) )
s32 Cosm_ParseFloat( void * number, utf8 ** end, const utf8 * string,
  u8 number_width );
  /*
    Worker function for Cosm{float}Str functions, places the parsed value
    into number, when number_width is the bytes in the expected value (4, 8).
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */
#endif

s32 Cosm_GetCharType( utf8char ch );
  /*
    Helper function for Cosm_Parse. Maps characters into the below.
    Returns:
      COSM_IO_DIGIT_SIGN     sign(+/-)
      COSM_IO_DIGIT_INVALID  invalid
      COSM_IO_DIGIT_SPACE    whitespace
      COSM_IO_DIGIT_DOT      decimal separator
      COSM_IO_DIGIT_E        'e' or 'E'
      COSM_IO_DIGIT_X        'x' or 'X'
      >= 0                 a number - valid or not depending on radix.
  */

u32 Cosm_Print( cosm_FILE * file, void * string, u32 max_bytes,
  const void * format, va_list args );
  /*
    Output the formatted character string to the file or string.
    If file is not NULL, then we write to the file.
    If string_ptr is not NULL, then we output to the string.
    If both are NULL, we write to the standard output.
    Returns: Number of characters written to file or string.
  */

s32 Cosm_PrintChar( u32 * bytes, cosm_FILE * file, void * string,
  u32 max_bytes, utf8char character );
  /*
    Output the character to the file or string, but to max_bytes worth.
    bytes is set to the number of bytes used.
    If file is not NULL, then we write to the file.
    If string_ptr is not NULL, then we output to the string.
    If both are NULL, we write to the standard output.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* testing */

s32 Cosm_TestOSIO( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
