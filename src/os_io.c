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

/* CPU/OS Layer - CPU and OS specific code is allowed */

#include "cosm/os_mem.h"
#include "cosm/os_io.h"
#include <stdio.h>

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#include <io.h>
#include <conio.h>
#else
#include <unistd.h>
#if ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_ANDROID ) \
  || ( OS_TYPE == OS_OSX ) || ( OS_TYPE == OS_IOS ) \
  || ( OS_TYPE == OS_SOLARIS ) || ( OS_TYPE == OS_FREEBSD ) \
  || ( OS_TYPE == OS_OPENBSD ) || ( OS_TYPE == OS_NETBSD ) )
#define UNIX_NOECHO_METHOD
#include <termios.h>
#endif
#endif

/* some systems aren't ISO compliant */
#if ( !defined( floorl ) )
#define floorl floor
#endif
#if ( !defined( fmodl ) )
#define fmodl fmod
#endif

/* states for the print state machine */
enum COSM_IO_STATES
{
  COSM_IO_TEXT,
  COSM_IO_PERCENT,
  COSM_IO_WIDTH,
  COSM_IO_PREC,
  COSM_IO_SINGLE_CHAR,
  COSM_IO_STRING,
  COSM_IO_BUFFER,
  COSM_IO_TMP_STRING,
  COSM_IO_U32_DEC,
  COSM_IO_U64_DEC,
  COSM_IO_U128_DEC,
  COSM_IO_S32_DEC,
  COSM_IO_S64_DEC,
  COSM_IO_S128_DEC,
  COSM_IO_U32_HEX,
  COSM_IO_U64_HEX,
  COSM_IO_U128_HEX,
  COSM_IO_F64,
  COSM_IO_F64_SCI,
  COSM_IO_POINTER
};

static const ascii hex_table[16] =
{
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

#define COSM_IO_NO 0
#define COSM_IO_YES 1

u32 CosmInputRaw( utf8 * buffer, u32 length, u32 echo )
{
  u32 chars_read;
  int tmp;
  int done;
#if ( defined( UNIX_NOECHO_METHOD ) )
  struct termios new_ts, old_ts;
#endif

  chars_read = 0;

  if ( ( buffer == NULL ) || ( length == 0 )
   || ( ( echo != COSM_IO_ECHO ) && ( echo != COSM_IO_NOECHO ) ) )
  {
    return chars_read;
  }

  /* make sure we are not buffering input */
  setvbuf( stdin, NULL, _IONBF, 0 );

  /* take care of unix settings */
#if ( defined( UNIX_NOECHO_METHOD ) )
  /* save current settings */
  tcgetattr( STDIN_FILENO, &old_ts );
  new_ts = old_ts;

  /* turn off ICANON - part 2 of no buffering */
  new_ts.c_lflag &= ~ICANON;

  if ( echo == COSM_IO_NOECHO )
  {
    new_ts.c_lflag &= ~ECHO;
    new_ts.c_lflag |= ECHONL;
    tcsetattr( STDIN_FILENO, TCSANOW, &new_ts );

    /* make sure we could set them */
    tcgetattr( STDIN_FILENO, &new_ts );
    if ( ( new_ts.c_lflag & ECHO ) == ECHO )
    {
      return chars_read;
    }
  }
  else /* echo */
  {
    new_ts.c_lflag |= ECHO;
    new_ts.c_lflag |= ECHONL;
    tcsetattr( STDIN_FILENO, TCSANOW, &new_ts );

    /* make sure we could set them */
    tcgetattr( STDIN_FILENO, &new_ts );
    if ( ( new_ts.c_lflag & ECHO ) != ECHO )
    {
      return chars_read;
    }
  }
#endif

  done = COSM_IO_NO;
  while ( ( chars_read < length ) && ( done == COSM_IO_NO ) )
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    /* in Win32 and friends, getch() doesn't do echo */
    if ( echo == COSM_IO_NOECHO )
    {
      tmp = (int) _getch();
    }
    else
    {
      tmp = fgetc( stdin );
    }
#else
    tmp = fgetc( stdin );
#endif

    /* read until the linefeed, place chars in buffer if under limit */
    switch ( tmp )
    {
      case -1: /* EOF */
        if ( chars_read == 0 )
        {
          /* no characters read yet, then EOF */
          chars_read = 0xFFFFFFFF;
        }
        done = COSM_IO_YES;
        break;
      default:
        *buffer++ = (ascii) tmp;
        chars_read++;
        break;
    }
  }

#if ( defined( UNIX_NOECHO_METHOD ) )
  /* restore echo state and flush here */
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_ts );
#endif

  return chars_read;
}

u32 CosmInput( utf8 * buffer, u32 max_bytes, u32 echo )
{
  u32 chars_read;
  int tmp;
  int done;
#if ( defined( UNIX_NOECHO_METHOD ) )
  struct termios new_ts, old_ts;
#endif

  chars_read = 0x0000000000000000LL;

  /* Make sure that buffer is actually set to something */
  if ( ( buffer == NULL ) || ( max_bytes == 0 ) )
  {
    return chars_read;
  }

  /* dont overflow the buffer */
  max_bytes--;

  /* take care of unix echo setting */
#if ( defined( UNIX_NOECHO_METHOD ) )
  /* save settings */
  tcgetattr( STDIN_FILENO, &old_ts );
  new_ts = old_ts;
  if ( echo == COSM_IO_NOECHO )
  {
    new_ts.c_lflag &= ~ECHO;
    new_ts.c_lflag |= ECHONL;
    tcsetattr( STDIN_FILENO, TCSANOW, &new_ts );

    /* make sure we could set them */
    tcgetattr( STDIN_FILENO, &new_ts );
    if ( ( new_ts.c_lflag & ECHO ) == ECHO )
    {
      return chars_read;
    }
  }
  else /* echo */
  {
    new_ts.c_lflag |= ECHO;
    new_ts.c_lflag |= ECHONL;
    tcsetattr( STDIN_FILENO, TCSANOW, &new_ts );

    /* make sure we could set them */
    tcgetattr( STDIN_FILENO, &new_ts );
    if ( ( new_ts.c_lflag & ECHO ) != ECHO )
    {
      return chars_read;
    }
  }
#endif

  done = COSM_IO_NO;
  while ( done == COSM_IO_NO )
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    /* in Win32 and friends, getch() doesn't do echo */
    if ( echo == COSM_IO_NOECHO )
    {
      tmp = (int) _getch();
    }
    else
    {
      tmp = fgetc( stdin );
    }
#else
    tmp = fgetc( stdin );
#endif

    /* read until the linefeed, place chars in buffer if under limit */
    switch ( tmp )
    {
      case -1: /* EOF */
        if ( chars_read == 0 )
        {
          /* no characters read yet, then EOF */
          chars_read = 0xFFFFFFFF;
        }
        done = COSM_IO_YES;
        break;
      case '\r': /* Carriage Return - on Windows */
        fwrite( "\n", 1, 1, stdout );
        /* fall through */
      case '\n': /* Line Feed - on anything else */
        done = COSM_IO_YES;
        break;
      default:
        if ( chars_read < max_bytes )
        {
          *buffer++ = (ascii) tmp;
          chars_read++;
        }
        break;
    }
  }

#if ( defined( UNIX_NOECHO_METHOD ) )
  /* restore echo state and flush here */
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_ts );
#endif

  /* terminate string and leave */
  *buffer = 0;
  return chars_read;
}

u32 CosmStrBytes( const utf8 * string )
{
  u32 length;

  length = 0;

  if ( string == NULL )
  {
    return length;
  }

  while ( *(string)++ != 0 )
  {
    length++;
  }

  return length;
}

s32 CosmStrCopy( utf8 * dest, const utf8 * src, u32 max_bytes )
{
  u32 length;

  if ( ( dest == NULL ) || ( src == NULL ) || ( max_bytes == 0 ) )
  {
    return COSM_FAIL;
  }

  /* check for ( max_bytes - 1 ) */
  length = CosmStrBytes( src );
  max_bytes--;

  if ( length > max_bytes )
  {
    return COSM_FAIL;
  }

  while ( length > 0 )
  {
    *(dest)++ = *(src)++;
    length--;
  }
  *dest = 0;

  return COSM_PASS;
}

s32 CosmStrAppend( utf8 * stringA, const utf8 * stringB, u32 max_bytes )
{
  u32 lenA, lenB;
  utf8 * ptr;

  if ( ( stringA == NULL ) || ( stringB == NULL ) || ( max_bytes == 0 ) )
  {
    return COSM_FAIL;
  }

  lenA = CosmStrBytes( stringA );
  lenB = CosmStrBytes( stringB );

  max_bytes--;

  if ( ( ( lenA + lenB ) < lenA )
    || ( ( lenA + lenB ) > max_bytes ) )
  {
    /* u32 wrap or too long */
    return COSM_FAIL;
  }

  ptr = CosmMemOffset( stringA, lenA );

  /* Check to see if we exceded bounds */
  if ( ptr == 0 )
  {
    return COSM_FAIL;
  }

  /* Copy stringB onto the end of stringA */
  while ( *stringB != 0 )
  {
    *(ptr)++ = *(stringB)++;
  }
  *ptr = 0;

  return COSM_PASS;
}

s32 CosmStrCmp( const utf8 * stringA, const utf8 * stringB, u32 max_bytes )
{
  u32 i;
  s32 a, b;

  if ( ( stringA == NULL ) || ( stringB == NULL ) || ( max_bytes == 0 ) )
  {
    if ( stringA != stringB )
    {
      return -1;
    }
    return 0;
  }

  i = 0;
  do
  {
    a = (s32) *(stringA)++;
    b = (s32) *(stringB)++;
    i++;
  } while ( ( a == b ) && ( a != 0 ) && ( i < max_bytes ) );

  return a - b;
}

utf8 * CosmStrChar( const utf8 * string, utf8char character, u32 max_bytes )
{
  utf8 array[8];
  u32 bytes;

  if ( ( string == NULL ) || ( max_bytes == 0 ) )
  {
    return NULL;
  }

  if ( Cosm_EncodeUTF8( array, &bytes, character ) == COSM_FAIL )
  {
    return NULL;
  }
  array[bytes] = 0;

  return CosmStrStr( string, array, max_bytes );
}

utf8 * CosmStrStr( const utf8 * string, const utf8 * substring, u32 max_bytes )
{
  utf8 * first, * end, * str, * sub;
  utf8 ch;
  u32 len;

  /* check for bad or trivial parameters */
  if ( ( string == NULL ) || ( substring == NULL ) ||
    ( string[0] == 0 ) || ( substring[0] == 0 ) )
  {
    return NULL;
  }

  /* substring too long? */
  if ( ( len = CosmStrBytes( substring ) ) > ( max_bytes - 1 ) )
  {
    return NULL;
  }

  /*
    end is set to the last character substring could safely start at.
    makes it safe and saves some iterations too.
  */
  end = (utf8 *) CosmMemOffset( string, max_bytes - len );
  first = (utf8 *) string;
  ch = substring[0];

  for ( ; ; )
  {
    /* find a matching first character in string */
    while ( *first != ch )
    {
      if ( *first == 0 )
      {
        return NULL;
      }
      first++;
      if ( first > end )
      {
        return NULL;
      }
    }

    str = first;
    sub = (utf8 *) substring;

    /* match up the strings */
    while ( ( *str == *sub ) && ( *sub != 0 ) )
    {
      str++;
      sub++;
    }

    /* if we got safely to the end of sub, then we found it */
    if ( *sub == 0 )
    {
      return first;
    }

    first++;
    if ( first > end )
    {
      return NULL;
    }
  }
}

u32 CosmPrint( const utf8 * format, ... )
{
  u32 chars_output;
  va_list ap;

  if ( format == NULL )
  {
    return 0;
  }

  va_start( ap, format );
  chars_output = Cosm_Print( NULL, NULL, 0xFFFFFFFF, format, ap );
  va_end( ap );

  fflush( NULL );

  return chars_output;
}

u32 CosmPrintStr( utf8 * string, u32 max_bytes, const utf8 * format, ... )
{
  u32 chars_output;
  va_list ap;

  if ( ( string == NULL ) || ( max_bytes == 0 ) )
  {
    return 0;
  }

  max_bytes--;

  va_start( ap, format );
  chars_output = Cosm_Print( NULL, string, max_bytes, format, ap );
  va_end( ap );

  /* terminate string */
  *( (utf8 *) CosmMemOffset( string, (u64) chars_output ) ) = 0;

  return chars_output;
}

u32 CosmPrintFile( cosm_FILE * file, const utf8 * format, ... )
{
  u32 chars_output;
  va_list ap;

  if ( file == NULL )
  {
    return 0;
  }

  va_start( ap, format );
  chars_output = Cosm_Print( file, NULL, 0xFFFFFFFF, format, ap );
  va_end( ap );

  return chars_output;
}

/* Low level functions */

s32 Cosm_DecodeUTF8( utf8char * codepoint, utf8 ** next, utf8 * stream )
{
  utf8char code;
  u32 length;

  if ( ( codepoint == NULL ) || ( stream == NULL ) )
  {
    return COSM_FAIL;
  }

  /*
    Decodings:
    1  U-00000000 - U-0000007F : 0xxxxxxx
    2  U-00000080 - U-000007FF : 110xxxxx   10xxxxxx
    3  U-00000800 - U-0000FFFF : 1110xxxx ( 10xxxxxx * 2 )
    4  U-00010000 - U-001FFFFF : 11110xxx ( 10xxxxxx * 3 )
    5  U-00200000 - U-03FFFFFF : 111110xx ( 10xxxxxx * 4 )
    6  U-04000000 - U-7FFFFFFF : 1111110x ( 10xxxxxx * 5 )
  */

  /* 1 byte */
  if ( ( stream[0] & 0x80 ) == 0x00 )
  {
    code = *stream;
    length = 1;
  }
  /* 2 bytes */
  else if ( ( stream[0] & 0xE0 ) == 0xC0 )
  {
    if ( ( ( stream[1] ) & 0xC0 ) != 0x80 )
    {
      return COSM_FAIL;
    }
    code = stream[0] & 0x1F;
    code = ( code << 6 ) | ( stream[1] & 0x3F );
    if ( code < 0x80 )
    {
      return COSM_FAIL;
    }
    length = 2;
  }
  /* 3 bytes */
  else if ( ( stream[0] & 0xF0 ) == 0xE0 )
  {
    if ( ( ( ( stream[1] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[2] ) & 0xC0 ) != 0x80 ) )
    {
      return COSM_FAIL;
    }
    code = stream[0] & 0x0F;
    code = ( code << 6 ) | ( stream[1] & 0x3F );
    code = ( code << 6 ) | ( stream[2] & 0x3F );
    if ( code < 0x800 )
    {
      return COSM_FAIL;
    }
    length = 3;
  }
  /* 4 bytes */
  else if ( ( stream[0] & 0xF8 ) == 0xF0 )
  {
    if ( ( ( ( stream[1] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[2] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[3] ) & 0xC0 ) != 0x80 ) )
    {
      return COSM_FAIL;
    }
    code = stream[0] & 0x07;
    code = ( code << 6 ) | ( stream[1] & 0x3F );
    code = ( code << 6 ) | ( stream[2] & 0x3F );
    code = ( code << 6 ) | ( stream[3] & 0x3F );
    if ( code < 0x10000 )
    {
      return COSM_FAIL;
    }
    length = 4;
  }
  /* 5 bytes */
  else if ( ( stream[0] & 0xFC ) == 0xF8 )
  {
    if ( ( ( ( stream[1] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[2] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[3] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[4] ) & 0xC0 ) != 0x80 ) )
    {
      return COSM_FAIL;
    }
    code = stream[0] & 0x03;
    code = ( code << 6 ) | ( stream[1] & 0x3F );
    code = ( code << 6 ) | ( stream[2] & 0x3F );
    code = ( code << 6 ) | ( stream[3] & 0x3F );
    code = ( code << 6 ) | ( stream[4] & 0x3F );
    if ( code < 0x200000 )
    {
      return COSM_FAIL;
    }
    length = 5;
  }
  /* 6 bytes */
  else if ( ( stream[0] & 0xFE ) == 0xFC )
  {
    if ( ( ( ( stream[1] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[2] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[3] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[4] ) & 0xC0 ) != 0x80 )
      || ( ( ( stream[5] ) & 0xC0 ) != 0x80 ) )
    {
      return COSM_FAIL;
    }
    code = stream[0] & 0x01;
    code = ( code << 6 ) | ( stream[1] & 0x3F );
    code = ( code << 6 ) | ( stream[2] & 0x3F );
    code = ( code << 6 ) | ( stream[3] & 0x3F );
    code = ( code << 6 ) | ( stream[4] & 0x3F );
    code = ( code << 6 ) | ( stream[5] & 0x3F );
    if ( code < 0x4000000 )
    {
      return COSM_FAIL;
    }
    length = 6;
  }
  else
  {
    return COSM_FAIL;
  }

  *codepoint = code;

  if ( next != NULL )
  {
    *next = &stream[length];
  }

  return COSM_PASS;
}

s32 Cosm_EncodeUTF8( utf8 * array, u32 * bytes_used, utf8char codepoint )
{
  u32 len;

  if ( ( array == NULL ) || ( bytes_used == NULL ) )
  {
    return COSM_FAIL;
  }

  /*
    Encodings:

    U-00000000 - U-0000007F => 0xxxxxxx
    U-00000080 - U-000007FF => 110xxxxx   10xxxxxx
    U-00000800 - U-0000FFFF => 1110xxxx ( 10xxxxxx * 2 )
    U-00010000 - U-001FFFFF => 11110xxx ( 10xxxxxx * 3 )
    U-00200000 - U-03FFFFFF => 111110xx ( 10xxxxxx * 4 )
    U-04000000 - U-7FFFFFFF => 1111110x ( 10xxxxxx * 5 )
  */

  if ( codepoint < 0x80 )
  {
    array[0] = (utf8) codepoint;
    len = 1;
  }
  else if ( codepoint < 0x800 )
  {
    array[0] = (utf8) ( 0xC0 | ( codepoint >> 6 ) );
    array[1] = (utf8) ( 0x80 | ( codepoint & 0x3F ) );
    len = 2;
  }
  else if ( codepoint < 0x10000 )
  {
    array[0] = (utf8) ( 0xE0 |   ( codepoint >> 12 ) );
    array[1] = (utf8) ( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
    array[2] = (utf8) ( 0x80 |   ( codepoint & 0x3F ) );
    len = 3;
  }
  else if ( codepoint < 0x200000 )
  {
    array[0] = (utf8) ( 0xF0 |   ( codepoint >> 18 ) );
    array[1] = (utf8) ( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) );
    array[2] = (utf8) ( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
    array[3] = (utf8) ( 0x80 |   ( codepoint & 0x3F ) );
    len = 4;
  }
  else if ( codepoint < 0x4000000 )
  {
    array[0] = (utf8) ( 0xF8 |   ( codepoint >> 24 ) );
    array[1] = (utf8) ( 0x80 | ( ( codepoint >> 18 ) & 0x3F ) );
    array[2] = (utf8) ( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) );
    array[3] = (utf8) ( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
    array[4] = (utf8) ( 0x80 |   ( codepoint & 0x3F ) );
    len = 5;
  }
  else if ( codepoint < 0x80000000 )
  {
    array[0] = (utf8) ( 0xFC |   ( codepoint >> 30 ) );
    array[1] = (utf8) ( 0x80 | ( ( codepoint >> 24 ) & 0x3F ) );
    array[2] = (utf8) ( 0x80 | ( ( codepoint >> 18 ) & 0x3F ) );
    array[3] = (utf8) ( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) );
    array[4] = (utf8) ( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
    array[5] = (utf8) ( 0x80 |   ( codepoint & 0x3F ) );
    len = 6;
  }
  else
  {
    return COSM_FAIL;
  }

  *bytes_used = len;

  return COSM_PASS;
}

/* temp macro for getting next character */
#define _COSM_IO_FETCHCHAR() { \
  possible_end = stream; \
  if ( Cosm_DecodeUTF8( &character, &stream, stream ) != COSM_PASS ) \
  { \
    return COSM_FAIL;\
  } }

s32 Cosm_ParseInt( void * number, utf8 ** end, const utf8 * string,
  u32 number_width, u32 sign, u32 radix )
{
  u32 tmp_u32, max_u32s, max_rem;
  u64 tmp_u64, max_u64s, rad_u64;
  u128 tmp_u128, max_u128S, rad_u128;
  const void * start;
  utf8char character;
  utf8 * stream;
  utf8 * possible_end;
  u32 minus;
  s32 val;

  if ( ( number == NULL ) || ( string == NULL ) || ( radix > 36 ) ||
    ( radix == 1 ) )
  {
    return COSM_FAIL;
  }

  stream = (utf8 *) string;

  _COSM_IO_FETCHCHAR();

  /*
    First we expect an optional sequence of spaces
  */
  while ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_SPACE )
  {
    _COSM_IO_FETCHCHAR();
  }

  /* sign? (val -1) */
  minus = 0;
  if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_SIGN )
  {
    if ( character == '-' )
    {
      if ( sign == COSM_IO_SIGNED )
      {
        minus = 1;
      }
      else
      {
        return COSM_FAIL;
      }
    }
    _COSM_IO_FETCHCHAR();
  }

  /* determine radix if we need to */
  if ( radix == 0 )
  {
    if ( character == '0' )
    {
      _COSM_IO_FETCHCHAR();
      if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_X )
      {
        radix = 16;
      }
      else
      {
        radix = 8;
      }
      _COSM_IO_FETCHCHAR();
    }
    else
    {
      radix = 10;
    }
  }

  /*
    parse the numbers until we hit something with val above radix or < 0.
    setup limits, and start eatting numbers:
      max_...s is the max a number can be and still shift.
      max_rem is the max final remainder we can have at that last shift.
  */
  start = stream;
  val = Cosm_GetCharType( character );
  switch ( number_width )
  {
    case 4:
      tmp_u32 = 0;
      max_u32s = 0xFFFFFFFF;
      if ( sign == COSM_IO_SIGNED )
      {
        max_u32s >>= 1;
      }
      max_rem = max_u32s % radix;
      max_u32s = max_u32s / radix;
      /* eat numbers */
      while ( ( val >= 0 ) && ( val < (s32) radix ) )
      {
        if ( ( tmp_u32 > max_u32s ) ||
          ( ( tmp_u32 == max_u32s ) && ( val > (s32) max_rem ) ) )
        {
          /* too many characters */
          return COSM_FAIL;
        }
        tmp_u32 *= radix;
        tmp_u32 += val;
        _COSM_IO_FETCHCHAR();
        val = Cosm_GetCharType( character );
      }
      /* fix sign, set number */
      if ( minus == 1 )
      {
        tmp_u32 = ~tmp_u32 + 1;
      }
      *( (u32 *) number ) = tmp_u32;
      break;
    case 8:
      tmp_u64 = 0x0000000000000000LL;
      max_u64s = 0xFFFFFFFFFFFFFFFFLL;
      if ( sign == COSM_IO_SIGNED )
      {
        max_u64s = ( max_u64s >> 1 );
      }
      rad_u64 = (u64) radix;
      max_rem = (u32) ( max_u64s % rad_u64 );
      max_u64s = ( max_u64s / rad_u64 );
      /* eat numbers */
      while ( ( val >= 0 ) && ( val < (s32) radix ) )
      {
        if ( ( tmp_u64 > max_u64s ) ||
          ( ( tmp_u64 == max_u64s ) && ( val > (s32) max_rem ) ) )
        {
          /* too many characters */
          return COSM_FAIL;
        }
        tmp_u64 = ( ( tmp_u64 * rad_u64 ) + (u64) val );
        _COSM_IO_FETCHCHAR();
        val = Cosm_GetCharType( character );
      }
      /* fix sign, set number */
      if ( minus == 1 )
      {
        tmp_u64 = ( ~tmp_u64 ) + 1LL;
      }
      *( (u64 *) number ) = tmp_u64;
      break;
    case 16:
      _COSM_SET128( tmp_u128, 0000000000000000, 0000000000000000 );
      _COSM_SET128( max_u128S, FFFFFFFFFFFFFFFF, FFFFFFFFFFFFFFFF );
      if ( sign == COSM_IO_SIGNED )
      {
        max_u128S = CosmU128Rsh( max_u128S, 1 );
      }
      rad_u128 = CosmU128U32( radix );
      max_rem = CosmU32U128( CosmU128Mod( max_u128S, rad_u128 ) );
      max_u128S = CosmU128Div( max_u128S, rad_u128 );
      /* eat numbers */
      while ( ( val >= 0 ) && ( val < (s32) radix ) )
      {
        if ( ( CosmU128Gt( tmp_u128, max_u128S ) ) ||
          ( ( CosmU128Eq( tmp_u128, max_u128S ) ) && ( val > (s32) max_rem ) ) )
        {
          /* too many characters */
          return COSM_FAIL;
        }
        tmp_u128 = CosmU128Add( CosmU128Mul( tmp_u128, rad_u128 ),
          CosmU128U32( val ) );
        _COSM_IO_FETCHCHAR();
        val = Cosm_GetCharType( character );
      }
      /* fix sign, set number */
      if ( minus == 1 )
      {
        tmp_u128 = CosmU128Add( CosmU128Not( tmp_u128 ), CosmU128U32( 1 ) );
      }
      *( (u128 *) number ) = tmp_u128;
      break;
    default:
      return COSM_FAIL;
  }

  /* was there a number at all? */
  if ( start == stream )
  {
    return COSM_FAIL;
  }

  /* set end if we need to */
  if ( end != NULL )
  {
    *end = (utf8 *) possible_end;
  }

  return COSM_PASS;
}

s32 Cosm_ParseFloat( void * number, utf8 ** end, const utf8 * string,
  u8 number_width )
{
  f64 fraction, exponent, position_value;
  f64 result;
  utf8char character;
  s32 sign, esign;
  s32 pass;
  utf8 * stream;
  utf8 * possible_end;

  if ( ( number == NULL ) || ( string == NULL ) )
  {
    return COSM_FAIL;
  }

  stream = (utf8 *) string;

  switch ( number_width )
  {
    case 4:
      *(f32 *) number = (f32) 0;
      break;
    case 8:
      *(f64 *) number = (f64) 0;
      break;
    default:
      return COSM_FAIL;
      break;
  }

  result = 0;

  _COSM_IO_FETCHCHAR();

  /*
    First we expect an optional sequence of spaces
  */
  while ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_SPACE )
  {
    _COSM_IO_FETCHCHAR();
  }

  /* An optional sign. A number is implicitly positive. */
  sign = 1;
  if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_SIGN )
  {
    if ( character == '-' )
    {
      sign = -1;
    }
    _COSM_IO_FETCHCHAR();
  }

  /* Now we expect the integer part of the fraction. */
  fraction = 0;
  while ( ( Cosm_GetCharType( character ) >= 0 )
    && ( Cosm_GetCharType( character ) < 10 ) )
  {
    fraction = ( fraction * 10 ) + Cosm_GetCharType( character );
    _COSM_IO_FETCHCHAR();
  }

  /* Then an optional period */
  if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_DOT )
  {
    _COSM_IO_FETCHCHAR();
    position_value = 0.1;
    /* If we got a period then we want an optional number*/
    while ( ( Cosm_GetCharType( character ) >= 0 )
      && ( Cosm_GetCharType( character ) < 10 ) )
    {
      fraction += position_value * Cosm_GetCharType( character );
      position_value /= 10;
      _COSM_IO_FETCHCHAR();
    }
  }

  exponent = 0;
  esign = 1;

  /* Then there is an optional 'exponent-package.' */
  if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_E )
  {
    /* Skip the E. */
    _COSM_IO_FETCHCHAR();

    /* Then an optional sign just as for the fraction */
    if ( Cosm_GetCharType( character ) == COSM_IO_DIGIT_SIGN )
    {
      if ( character == '-' )
      {
        esign = -1;
      }
      _COSM_IO_FETCHCHAR();
    }

    exponent = 0;
    /* This number is required. */
    if ( ( ( Cosm_GetCharType( character ) < 0 )
      || ( Cosm_GetCharType( character ) > 9 ) ) )
    {
      return COSM_FAIL;
    }

    while ( ( Cosm_GetCharType( character ) >= 0 )
      && ( Cosm_GetCharType( character ) < 10 ) )
    {
      exponent = ( exponent * 10 ) + Cosm_GetCharType( character );
      _COSM_IO_FETCHCHAR();
    }
  }

  result = sign * ( fraction * pow( 10, esign * exponent ) );

  pass = COSM_PASS;

  /* Too large a number. Positive or negative. */
  if ( CosmFloatInf( result ) )
  {
    if ( result < 0 )
    {
      result = -HUGE_VAL;
    }
    else
    {
      result = HUGE_VAL;
    }
    pass = COSM_FAIL;
  }

  if ( CosmFloatNaN( result ) )
  {
    result = 0;
    pass = COSM_FAIL;
  }

  if ( number_width == 4 )
  {
    *((f32 *) number) = (f32) result;
  }
  else
  {
    *((f64 *) number) = (f64) result;
  }

  /* set end if we need to */
  if ( end != NULL )
  {
    *end = (utf8 *) possible_end;
  }

  return pass;
}

#undef _COSM_IO_FETCHCHAR

s32 Cosm_GetCharType( utf8char ch )
{
  static const s8 num[256] =
  {
    -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -3, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -2, -1, -4, -2,
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -2, -2, -2, -2, -2, -2,
    -2, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -2, -2, -2, -2, -2,
    -2, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -3, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
  };
  s8 val;

  if ( ch < 256 )
  {
    return num[(u8) ch];
  }
  else
  {
    /*
      AUTO GENERATED FROM
      ftp://ftp.Unicode.org/Public/UNIDATA/UnicodeData-Latest.txtpub
      on 7/15/00
      Then hand edited for sanity. Then added to by hand because the layout
      of the symbols in each section is not consistant --HAA
    */

    /* whitespace */
    if ( ( ch == 0x1361 ) /* ETHIOPIC WORDSPACE */
      || ( ch == 0x1680 ) /* OGHAM SPACE MARK */
      || ( ( ch >= 0x2002 ) && ( ch <= 0x200B ) ) /* different size spaces */
      || ( ch == 0x202F ) /* NARROW NO-BREAK SPACE */
      || ( ch == 0x3000 ) /* IDEOGRAPHIC SPACE */
      || ( ch == 0x303F ) /* IDEOGRAPHIC HALF FILL SPACE */
      || ( ch == 0xFEFF ) ) /* ZERO WIDTH NO-BREAK SPACE */
    {
     val = -3;
    }
    /* GREEK CAPITAL LETTER ALPHA through GREEK CAPITAL LETTER UPSILON */
    else if ( ch >= 0x0391 && ch <= 0x03A5 )
    {
      val = (s8) ( ( ch - 0x0391 ) + 10 );
    }
    /* GREEK SMALL LETTER ALPHA through GREEK SMALL LETTER UPSILON */
    else if ( ch >= 0x03AC && ch <= 0x03C5 )
    {
      val = (s8) ( ( ch - 0x03AC ) + 10 );
    }
    /* ARMENIAN CAPITAL LETTER AYB through ARMENIAN CAPITAL LETTER FEH */
    else if ( ch >= 0x0531 && ch <= 0x0556 )
    {
      val = (s8) ( ( ch - 0x0531 ) + 10 );
    }
    /* ARMENIAN SMALL LETTER AYB through ARMENIAN SMALL LETTER FEH */
    else if ( ch >= 0x0561 && ch <= 0x0586 )
    {
      val = (s8) ( ( ch - 0x0561 ) + 10 );
    }
    /* HEBREW LETTER ALEF through HEBREW LETTER TAV */
    else if ( ch >= 0x05D0 && ch <= 0x05EA )
    {
      val = (s8) ( ( ch - 0x05D0 ) + 10 );
    }
    /* ARABIC LETTER ALEF through ARABIC LETTER YEH */
    else if ( ch >= 0x0627 && ch <= 0x064A )
    {
      val = (s8) ( ( ch - 0x0627 ) + 10 );
    }
    /* SYRIAC LETTER ALAPH through SYRIAC LETTER TAW */
    else if ( ch >= 0x0710 && ch <= 0x072C )
    {
      val = (s8) ( ( ch - 0x0710 ) + 10 );
    }
    /* THAANA LETTER HAA through THAANA LETTER WAAVU */
    else if ( ch >= 0x0780 && ch <= 0x07A5 )
    {
      val = (s8) ( ( ch - 0x0780 ) + 10 );
    }
    /* DEVANAGARI LETTER A through DEVANAGARI LETTER HA */
    else if ( ch >= 0x0905 && ch <= 0x0939 )
    {
      val = (s8) ( ( ch - 0x0905 ) + 10 );
    }
    /* BENGALI LETTER A through BENGALI LETTER HA */
    else if ( ch >= 0x0985 && ch <= 0x09B9 )
    {
      val = (s8) ( ( ch - 0x0985 ) + 10 );
    }
    /* GURMUKHI LETTER A through GURMUKHI LETTER HA */
    else if ( ch >= 0x0A05 && ch <= 0x0A39 )
    {
      val = (s8) ( ( ch - 0x0A05 ) + 10 );
    }
    /* GUJARATI LETTER A through GUJARATI LETTER HA */
    else if ( ch >= 0x0A85 && ch <= 0x0AB9 )
    {
      val = (s8) ( ( ch - 0x0A85 ) + 10 );
    }
    /* ORIYA LETTER A through ORIYA LETTER HA */
    else if ( ch >= 0x0B05 && ch <= 0x0B39 )
    {
      val = (s8) ( ( ch - 0x0B05 ) + 10 );
    }
    /* TAMIL LETTER A through TAMIL LETTER HA */
    else if ( ch >= 0x0B85 && ch <= 0x0BB9 )
    {
      val = (s8) ( ( ch - 0x0B85 ) + 10 );
    }
    /* TELUGU LETTER A through TELUGU LETTER HA */
    else if ( ch >= 0x0C05 && ch <= 0x0C39 )
    {
      val = (s8) ( ( ch - 0x0C05 ) + 10 );
    }
    /* KANNADA LETTER A through KANNADA LETTER HA */
    else if ( ch >= 0x0C85 && ch <= 0x0CB9 )
    {
      val = (s8) ( ( ch - 0x0C85 ) + 10 );
    }
    /* MALAYALAM LETTER A through MALAYALAM LETTER HA */
    else if ( ch >= 0x0D05 && ch <= 0x0D39 )
    {
      val = (s8) ( ( ch - 0x0D05 ) + 10 );
    }
    /* SINHALA LETTER AYANNA through SINHALA LETTER FAYANNA */
    else if ( ch >= 0x0D85 && ch <= 0x0DC6 )
    {
      val = (s8) ( ( ch - 0x0D85 ) + 10 );
    }
    /* THAI CHARACTER KO KAI through THAI CHARACTER HO NOKHUK */
    else if ( ch >= 0x0E01 && ch <= 0x0E2E )
    {
      val = (s8) ( ( ch - 0x0E01 ) + 10 );
    }
    /* LAO LETTER KO through LAO LETTER HO TAM */
    else if ( ch >= 0x0E81 && ch <= 0x0EAE )
    {
      val = (s8) ( ( ch - 0x0E81 ) + 10 );
    }
    /* TIBETAN LETTER KA through TIBETAN LETTER KSSA */
    else if ( ch >= 0x0F40 && ch <= 0x0F69 )
    {
      val = (s8) ( ( ch - 0x0F40 ) + 10 );
    }
    /* MYANMAR LETTER KA through MYANMAR LETTER AU */
    else if ( ch >= 0x1000 && ch <= 0x102A )
    {
      val = (s8) ( ( ch - 0x1000 ) + 10 );
    }
    /* GEORGIAN CAPITAL LETTER AN through GEORGIAN CAPITAL LETTER HOE */
    else if ( ch >= 0x10A0 && ch <= 0x10C5 )
    {
      val = (s8) ( ( ch - 0x10A0 ) + 10 );
    }
    /* GEORGIAN LETTER AN through GEORGIAN LETTER HOE */
    else if ( ch >= 0x10D0 && ch <= 0x10F5 )
    {
      val = (s8) ( ( ch - 0x10D0 ) + 10 );
    }
    /* CHEROKEE LETTER A through CHEROKEE LETTER YV */
    else if ( ch >= 0x13A0 && ch <= 0x13F4 )
    {
      val = (s8) ( ( ch - 0x13A0 ) + 10 );
    }
    /* OGHAM LETTER BEITH through OGHAM LETTER PEITH */
    else if ( ch >= 0x1681 && ch <= 0x169A )
    {
      val = (s8) ( ( ch - 0x1681 ) + 10 );
    }
    /* RUNIC LETTER FEHU FEOH FE F through RUNIC LETTER X */
    else if ( ch >= 0x16A0 && ch <= 0x16EA )
    {
      val = (s8) ( ( ch - 0x16A0 ) + 10 );
    }
    /* KHMER LETTER KA through KHMER LETTER QA */
    else if ( ch >= 0x1780 && ch <= 0x17A2 )
    {
      val = (s8) ( ( ch - 0x1780 ) + 10 );
    }
    /* MONGOLIAN LETTER A through MONGOLIAN LETTER CHI */
    else if ( ch >= 0x1820 && ch <= 0x1842 )
    {
      val = (s8) ( ( ch - 0x1820 ) + 10 );
    }

    /* ARABIC-INDIC DIGIT ZERO through ARABIC-INDIC DIGIT NINE */
    else if ( ch >= 0x0660 && ch <= 0x0669 )
    {
      val = (s8) ( ch - 0x0660 );
    }
    /*
      EXTENDED ARABIC-INDIC DIGIT ZERO through EXTENDED ARABIC-INDIC
      DIGIT NINE
    */
    else if ( ch >= 0x06F0 && ch <= 0x06F9 )
    {
      val = (s8) ( ch - 0x06F0 );
    }
    /* DEVANAGARI DIGIT ZERO through DEVANAGARI DIGIT NINE */
    else if ( ch >= 0x0966 && ch <= 0x096F )
    {
      val = (s8) ( ch - 0x0966 );
    }
    /* BENGALI DIGIT ZERO through BENGALI DIGIT NINE */
    else if ( ch >= 0x09E6 && ch <= 0x09EF )
    {
      val = (s8) ( ch - 0x09E6 );
    }
    /* GURMUKHI DIGIT ZERO through GURMUKHI DIGIT NINE */
    else if ( ch >= 0x0A66 && ch <= 0x0A6F )
    {
      val = (s8) ( ch - 0x0A66 );
    }
    /* GUJARATI DIGIT ZERO through GUJARATI DIGIT NINE */
    else if ( ch >= 0x0AE6 && ch <= 0x0AEF )
    {
      val = (s8) ( ch - 0x0AE6 );
    }
    /* ORIYA DIGIT ZERO through ORIYA DIGIT NINE */
    else if ( ch >= 0x0B66 && ch <= 0x0B6F )
    {
      val = (s8) ( ch - 0x0B66 );
    }
    /* TAMIL DIGIT ONE through TAMIL DIGIT NINE -- NOTE: No "ZERO" !!! */
    else if ( ch >= 0x0BE7 && ch <= 0x0BEF )
    {
      val = (s8) ( ch - 0x0BE6 );
    }
    /* TELUGU DIGIT ZERO through TELUGU DIGIT NINE */
    else if ( ch >= 0x0C66 && ch <= 0x0C6F )
    {
      val = (s8) ( ch - 0x0C66 );
    }
    /* KANNADA DIGIT ZERO through KANNADA DIGIT NINE */
    else if ( ch >= 0x0CE6 && ch <= 0x0CEF )
    {
      val = (s8) ( ch - 0x0CE6 );
    }
    /* MALAYALAM DIGIT ZERO through MALAYALAM DIGIT NINE */
    else if ( ch >= 0x0D66 && ch <= 0x0D6F )
    {
      val = (s8) ( ch - 0x0D66 );
    }
    /* THAI DIGIT ZERO through THAI DIGIT NINE */
    else if ( ch >= 0x0E50 && ch <= 0x0E59 )
    {
      val = (s8) ( ch - 0x0E50 );
    }
    /* LAO DIGIT ZERO through LAO DIGIT NINE */
    else if ( ch >= 0x0ED0 && ch <= 0x0ED9 )
    {
      val = (s8) ( ch - 0x0ED0 );
    }
    /* TIBETAN DIGIT ZERO through TIBETAN DIGIT NINE */
    else if ( ch >= 0x0F20 && ch <= 0x0F29 )
    {
      val = (s8) ( ch - 0x0F20 );
    }
    /* MYANMAR DIGIT ZERO through MYANMAR DIGIT NINE */
    else if ( ch >= 0x1040 && ch <= 0x1049 )
    {
      val = (s8) ( ch - 0x1040 );
    }
    /* ETHIOPIC DIGIT ONE through ETHIOPIC DIGIT NINE -- No "ZERO" Digit. */
    else if ( ch >= 0x1369 && ch <= 0x1371 )
    {
      val = (s8) ( ch - 0x1368 );
    }
    /* KHMER DIGIT ZERO through KHMER DIGIT NINE */
    else if ( ch >= 0x17E0 && ch <= 0x17E9 )
    {
      val = (s8) ( ch - 0x17E0 );
    }
    /* MONGOLIAN DIGIT ZERO through MONGOLIAN DIGIT NINE */
    else if ( ch >= 0x1810 && ch <= 0x1819 )
    {
      val = (s8) ( ch - 0x1810 );
    }
    else
    {
      val = -2;
    }

    return val;
  }
}

/* temp macro to reset tmp variables in Cosm_Print */
#define _COSM_IO_CLEARTMP() { \
  state = COSM_IO_TEXT; \
  width = 0; \
  width_set = COSM_IO_NO; \
  pad_left = COSM_IO_NO; \
  pad_zero = COSM_IO_NO; \
  precision = 6; \
  precision_set = COSM_IO_NO; \
  tmp_count = 0; \
  tmp_sign = 0; \
  ptr_ascii = &tmp_ascii[0]; }

/* temp macro for making sure we don't print too much */
#define _COSM_IO_CHECKMAX() { \
  max_bytes -= bytes; \
  chars_output += bytes; \
  if ( max_bytes == 0 ) \
    return chars_output; \
  if ( string != NULL ) \
  { \
    output_str += bytes; \
  } }

/* temp macro for fecthing the next UTF8 character */
#define _COSM_IO_FETCHCHAR() { \
  if ( Cosm_DecodeUTF8( &character, &format_str, format_str ) != COSM_PASS ) \
  { \
    return chars_output; \
  } }

u32 Cosm_Print( cosm_FILE * file, void * string, u32 max_bytes,
  const void * format, va_list args )
{
  enum COSM_IO_STATES state;
  u32  chars_output;
  utf8char  character;
  u32 bytes;

  utf8 * format_str;
  utf8 * output_str;

  u32  tmp_u32;
  s32  tmp_s32;
  u64  tmp_u64;
  s64  tmp_s64;
  u128 tmp_u128;
  s128 tmp_s128;
  f64  tmp_f64;
  f64  whole_f64;
  f64  tmp_whole_f64;

  ascii * ptr_ascii;
  utf8 * ptr_utf8;
  ascii tmp_ascii[64]; /* longest is s128 at 40+sign */
  u32 tmp_count;
  s32 tmp_sign;
  s32 i;
  u32 u;
  u8 * ptr_u8;

  u32 width;
  u32 width_set;
  u32 pad_left;
  u32 pad_zero;
  u32 precision;
  u32 precision_set;
  u32 done;

  done = COSM_IO_NO;
  chars_output = 0;

  /* check for valid parameters */
  if ( ( max_bytes == chars_output ) || ( format == NULL ) )
  {
    return chars_output;
  }

  format_str = (utf8 *) format;
  output_str = (utf8 *) string;

  _COSM_IO_CLEARTMP();
  _COSM_IO_FETCHCHAR();

  if ( character == 0 )
  {
    return chars_output;
  }

  while ( done == COSM_IO_NO )
  {
    switch ( state )
    {
      case COSM_IO_TEXT:
        switch ( character )
        {
          case '%':
            state = COSM_IO_PERCENT;
            break;
          case 0:
            return chars_output;
          default:
            if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
              character ) == COSM_FAIL )
            {
              return chars_output;
            }
            _COSM_IO_CHECKMAX();
            break;
        }
        _COSM_IO_FETCHCHAR();
        break;
      case COSM_IO_PERCENT:
        if ( character == '%' )
        {
          if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
            character ) == COSM_FAIL )
          {
            return chars_output;
          }
          _COSM_IO_CHECKMAX();
          _COSM_IO_FETCHCHAR();
          _COSM_IO_CLEARTMP();
        }
        else
        {
          if ( ( character >= '1' ) && ( character <= '9' ) )
          {
            /* We are defining the width */
            width_set = COSM_IO_YES;
            state = COSM_IO_WIDTH;
          }
          else
          {
            switch ( character )
            {
              case '0':
                pad_zero = COSM_IO_YES;
                width_set = COSM_IO_YES;
                state = COSM_IO_WIDTH;
                break;
              case '-':
                pad_left = COSM_IO_YES;
                width_set = COSM_IO_YES;
                state = COSM_IO_WIDTH;
                break;
              case '*':
                width = va_arg( args, u32 );
                if ( width > 0 ) /* avoid nasty bug */
                  width_set = COSM_IO_YES;
                break;
              case '.':
                precision = 0;
                state = COSM_IO_PREC;
                break;
              case 'c':
                state = COSM_IO_SINGLE_CHAR;
                break;
              case 's':
                state = COSM_IO_STRING;
                break;
              case 'b':
                state = COSM_IO_BUFFER;
                break;
              case 'i':
                state = COSM_IO_S32_DEC;
                break;
              case 'j':
                state = COSM_IO_S64_DEC;
                break;
              case 'k':
                state = COSM_IO_S128_DEC;
                break;
              case 'u':
                state = COSM_IO_U32_DEC;
                break;
              case 'v':
                state = COSM_IO_U64_DEC;
                break;
              case 'w':
                state = COSM_IO_U128_DEC;
                break;
              case 'f':
                state = COSM_IO_F64;
                break;
              case 'F':
                state = COSM_IO_F64_SCI;
                break;
              case 'X':
                state = COSM_IO_U32_HEX;
                break;
              case 'Y':
                state = COSM_IO_U64_HEX;
                break;
              case 'Z':
                state = COSM_IO_U128_HEX;
                break;
              case 'p':
                state = COSM_IO_POINTER;
                break;
              case 0:
                return chars_output;
              default:
                state = COSM_IO_TEXT;
                break;
            }
            _COSM_IO_FETCHCHAR();
          }
        }
        break;
      case COSM_IO_WIDTH:
        switch ( character )
        {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            width = width * 10 + ( character - '0' );
            _COSM_IO_FETCHCHAR();
            break;
          case 0:
            return chars_output;
          default:
            state = COSM_IO_PERCENT;
            break;
         }
         break;
      case COSM_IO_PREC:
        switch ( character )
        {
          case '*':
            precision = va_arg( args, u32 );
            precision_set = COSM_IO_YES;
            _COSM_IO_FETCHCHAR();
            state = COSM_IO_PERCENT;
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            precision = precision * 10 + ( character - '0' );
            precision_set = COSM_IO_YES;
            _COSM_IO_FETCHCHAR();
            break;
          case 0:
            return chars_output;
          default:
            state = COSM_IO_PERCENT;
            break;
        }
        break;
      case COSM_IO_SINGLE_CHAR:
        if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
          va_arg( args, u32 ) ) == COSM_FAIL )
        {
          return chars_output;
        }
        _COSM_IO_CHECKMAX();
        _COSM_IO_CLEARTMP();
        break;
      case COSM_IO_STRING:
        ptr_utf8 = (utf8 *) va_arg( args, utf8 * );
        if ( ( precision_set ) && ( ptr_utf8 != NULL ) )
        {
          u = 0;
          while ( ( u < precision ) && ( ptr_utf8[u] != 0 ) )
          {
            if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
              ptr_utf8[u] ) == COSM_FAIL )
            {
              return chars_output;
            }
            _COSM_IO_CHECKMAX();
            u++;
          }
        }
        _COSM_IO_CLEARTMP();
        break;
      case COSM_IO_BUFFER:
        ptr_u8 = va_arg( args, u8 * );
        if ( ( precision_set ) && ( ptr_u8 != NULL ) )
        {
          for ( u = 0 ; u < precision ; u++ )
          {
            if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
              ptr_u8[u] ) == COSM_FAIL )
            {
              return chars_output;
            }
            _COSM_IO_CHECKMAX();
          }
        }
        _COSM_IO_CLEARTMP();
        break;
      case COSM_IO_TMP_STRING:
        /*
          print tmp_string backwards
          using tmp_count, width, pad_left, pad_zero
        */
        if ( ( tmp_sign ) && ( width_set ) )
        {
          width--;
        }
        /* First pad as necessary (zero pad or left pad ) */
        if ( ( width_set ) && ( width > tmp_count ) &&
          ( pad_left == COSM_IO_NO ) )
        {
          for ( u = 0 ; u < ( width - tmp_count ) ; u++ )
          {
            if ( pad_zero == COSM_IO_YES )
            {
              if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
                '0' ) == COSM_FAIL )
              {
                return chars_output;
              }
            }
            else
            {
              if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
                ' ' ) == COSM_FAIL )
              {
                return chars_output;
              }
            }
            _COSM_IO_CHECKMAX();
          }
        }
        if ( tmp_sign )
        {
          if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
            '-' ) == COSM_FAIL )
          {
            return chars_output;
          }
          _COSM_IO_CHECKMAX();
        }
        for ( i = ( tmp_count - 1 ) ; i > -1 ; i-- )
        {
          if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
            tmp_ascii[i] ) == COSM_FAIL )
          {
            return chars_output;
          }
          _COSM_IO_CHECKMAX();
        }
        if ( ( width_set ) && ( width > tmp_count ) &&
          ( pad_left == COSM_IO_YES ) )
        {
          for ( u = 0 ; u < ( width - tmp_count ) ; u++ )
          {
            if ( Cosm_PrintChar( &bytes, file, output_str, max_bytes,
              ' ' ) == COSM_FAIL )
            {
              return chars_output;
            }
            _COSM_IO_CHECKMAX();
          }
        }
        _COSM_IO_CLEARTMP();
        break;
      case COSM_IO_U32_DEC:
        tmp_u32 = va_arg( args, u32 );

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + ( tmp_u32 % 10 ) );
          tmp_u32 = tmp_u32 / 10;
          tmp_count++;
        } while ( tmp_u32 > 0 );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_U64_DEC:
        tmp_u64 = va_arg( args, u64 );

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + (u32) ( tmp_u64 % 10LL ) );
          tmp_u64 = ( tmp_u64 / 10LL );
          tmp_count++;
        } while ( ( tmp_u64 > 0 ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_U128_DEC:
        tmp_u128 = va_arg( args, u128 );

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + (u32) CosmU64U128(
            CosmU128Mod( tmp_u128, CosmU128U32( (u32) 10 ) ) ) );
          tmp_u128 = CosmU128Div( tmp_u128, CosmU128U32( (u32) 10 ) );
          tmp_count++;
        } while ( CosmU128Gt( tmp_u128, CosmU128U32( (u32) 0 ) ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_S32_DEC:
        tmp_s32 = va_arg( args, s32 );

        if ( tmp_s32 < 0 )
        {
          tmp_sign = -1;
          tmp_s32 = -tmp_s32;
        }

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + ( tmp_s32 % 10 ) );
          tmp_s32 = tmp_s32 / 10;
          tmp_count++;
        } while ( tmp_s32 > 0 );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_S64_DEC:
        tmp_s64 = va_arg( args, s64 );

        if ( ( tmp_s64 < 0 ) )
        {
          tmp_sign = -1;
          tmp_s64 = ( 0 - tmp_s64 );
        }

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + (s32) ( tmp_s64 % 10LL ) );
          tmp_s64 = ( tmp_s64 / 10LL );
          tmp_count++;
        } while ( ( tmp_s64 > 0 ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_S128_DEC:
        tmp_s128 = va_arg( args, s128 );

        if ( CosmS128Lt( tmp_s128, CosmS128S32( 0 ) ) )
        {
          tmp_sign = -1;
          tmp_s128 = CosmS128Sub( CosmS128S32( 0 ), tmp_s128 );
        }

        do
        {
          *ptr_ascii++ = (ascii) ( '0' + (u32) CosmS64S128(
            CosmS128Mod( tmp_s128, CosmS128S32( 10 ) ) ) );
          tmp_s128 = CosmS128Div( tmp_s128, CosmS128S32( 10 ) );
          tmp_count++;
        } while ( CosmS128Gt( tmp_s128, CosmS128S32( 0 ) ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_U32_HEX:
       tmp_u32 = va_arg( args, u32 );

        do
        {
          *ptr_ascii++ = hex_table[ tmp_u32 & 0xF ];
          tmp_u32 = tmp_u32 >> 4;
          tmp_count++;
        } while ( tmp_u32 > 0 );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_U64_HEX:
        tmp_u64 = va_arg( args, u64 );

        do
        {
          *ptr_ascii++ = hex_table[ (u32) ( tmp_u64 & 15LL ) ];
          tmp_u64 = ( tmp_u64 >> 4 );
          tmp_count++;
        } while ( ( tmp_u64 > 0 ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_U128_HEX:
        tmp_u128 = va_arg( args, u128 );

        do
        {
          *ptr_ascii++ = hex_table[ (u32) CosmU64U128(
            CosmU128And( tmp_u128, CosmU128U32( (u32) 15 ) ) ) ];
          tmp_u128 = CosmU128Rsh( tmp_u128, 4 );
          tmp_count++;
        } while ( CosmU128Gt( tmp_u128, CosmU128U32( 0 ) ) );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_F64:
        tmp_f64 = (f64) va_arg( args, f64 );

        if ( CosmFloatNaN( tmp_f64 ) )
        {
          *ptr_ascii++ = (ascii) 'N';
          *ptr_ascii++ = (ascii) 'a';
          *ptr_ascii++ = (ascii) 'N';
          *ptr_ascii++ = 0;
          tmp_count += 3;
          state = COSM_IO_TMP_STRING;
          break;
        }

        if ( tmp_f64 < 0 )
        {
          tmp_sign = -1;
          tmp_f64 = -tmp_f64;
        }

        if ( ( i = CosmFloatInf( tmp_f64 ) ) != 0 )
        {
          *ptr_ascii++ = (ascii) 'f';
          *ptr_ascii++ = (ascii) 'n';
          *ptr_ascii++ = (ascii) 'I';
          if ( i > 0 )
          {
            *ptr_ascii++ = (ascii) '+';
          }
          else
          {
            *ptr_ascii++ = (ascii) '-';
          }
          *ptr_ascii++ = 0;
          tmp_count += 4;
          state = COSM_IO_TMP_STRING;
          break;
        }

        /* Get whole/fraction part using casts. */
        whole_f64 = floor( tmp_f64 );
        tmp_f64 = tmp_f64 - whole_f64;

        /* Set precision to "6" if not set (From os_io.h)  */
        if ( precision_set )
        {
          if ( precision == 0 )
          {
            tmp_f64 = 0.0;
          }
          if ( precision > 40 )
          {
            precision = 40;
          }
        }
        else
        {
          precision = 6;
        }

        if ( tmp_f64 > 0 || precision )
        {
          for ( i = precision - 1 ; i >= 0 ; i-- )
          {
            if ( tmp_f64 > 0 )
            {
              tmp_f64 = tmp_f64 * 10.0;
              tmp_whole_f64 = floor( tmp_f64 );
              tmp_f64 = tmp_f64 - tmp_whole_f64;
            }
            else tmp_whole_f64 = 0;

            *(ptr_ascii+i) = (ascii) ( '0' + tmp_whole_f64 );
          }

          ptr_ascii += precision;
          tmp_count += precision;

          *ptr_ascii++ = (ascii) '.';
          tmp_count++;
        }

        /* print whole */
        do
        {
          *ptr_ascii++ = (ascii) ( '0' + fmod( whole_f64, 10.0 ) );
          whole_f64 = whole_f64 / 10;
          whole_f64 = floor( whole_f64 );
          tmp_count++;
        } while ( whole_f64 > 0 );

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_F64_SCI:
        tmp_f64 = (f64) va_arg( args, f64 );

        if ( CosmFloatNaN( tmp_f64 ) )
        {
          *ptr_ascii++ = (ascii) 'N';
          *ptr_ascii++ = (ascii) 'a';
          *ptr_ascii++ = (ascii) 'N';
          *ptr_ascii++ = 0;
          tmp_count += 3;
          state = COSM_IO_TMP_STRING;
          break;
        }

        if ( tmp_f64 < 0 )
        {
          tmp_sign = -1;
          tmp_f64 = -tmp_f64;
        }

        if ( ( i = CosmFloatInf( tmp_f64 ) ) != 0 )
        {
          *ptr_ascii++ = (ascii) 'f';
          *ptr_ascii++ = (ascii) 'n';
          *ptr_ascii++ = (ascii) 'I';
          if ( i > 0 )
          {
            *ptr_ascii++ = (ascii) '+';
          }
          else
          {
            *ptr_ascii++ = (ascii) '-';
          }
          *ptr_ascii++ = 0;
          tmp_count += 4;
          state = COSM_IO_TMP_STRING;
          break;
        }

        /* Count "E" digits */
        tmp_s32 = 0;
        if ( tmp_f64 != 0 )
        {
          while ( tmp_f64 < 1.0 )
          {
            tmp_s32--;
            tmp_f64 = tmp_f64 * 10.0;
          }
          while ( tmp_f64 >= 10.0 )
          {
            tmp_s32++;
            tmp_f64 = tmp_f64 / 10.0;
          }
        }

        if ( tmp_s32 )
        {
          if ( tmp_s32 < 0 )
          {
            tmp_u32 = -tmp_s32;
          }
          else
          {
            tmp_u32 = tmp_s32;
          }

          do
          {
            *ptr_ascii++ = (ascii) ( '0' + ( tmp_u32 % 10 ) );
            tmp_u32 = tmp_u32 / 10;
            tmp_count++;
          } while ( tmp_u32 > 0 );

          if ( tmp_s32 < 0 )
          {
            *ptr_ascii++ = (ascii) '-';
            tmp_count++;
          }

          *ptr_ascii++ = (ascii) 'E';
          tmp_count++;
        }

        whole_f64 = floor( tmp_f64 );
        tmp_f64 = tmp_f64 - whole_f64;

        /* Set precision to "6" if not set (From os_io.h)  */
        if ( precision_set )
        {
          if ( precision == 0 )
          {
            tmp_f64 = 0.0;
          }
          if ( precision > 40 )
          {
            precision = 40;
          }
        }
        else
        {
          precision = 6;
        }

        for ( i = precision - 1 ; i >= 0 ; i-- )
        {
          if ( tmp_f64 > 0 )
          {
            tmp_f64 = tmp_f64 * 10.0;
            tmp_whole_f64 = floor( tmp_f64 );
            tmp_f64 = tmp_f64 - tmp_whole_f64;
          }
          else
          {
            tmp_whole_f64 = 0;
          }

          *(ptr_ascii+i) = (ascii) ( '0' + tmp_whole_f64 );
        }
        ptr_ascii += precision;
        tmp_count += precision;

        *ptr_ascii++ = (ascii) '.';
        *ptr_ascii++ = (ascii) ( '0' + whole_f64 );
        tmp_count += 2;

        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
      case COSM_IO_POINTER:
        switch ( sizeof( void * ) )
        {
          case 4:
            tmp_u32 = (u32) va_arg( args, u32 );

            for ( u = 0 ; u < 8 ; u++ )
            {
              *ptr_ascii++ = hex_table[ tmp_u32 & 0x0F ];
              tmp_u32 = tmp_u32 >> 4;
              tmp_count++;
            }
            break;
          case 8:
            /*
              this will be wrong on little endian system with
              fake u64's and 64bit pointers, unlikely to exist.
            */
            tmp_u64 = va_arg( args, u64 );

            for ( u = 0 ; u < 16 ; u++ )
            {
              *ptr_ascii++ = hex_table[ (u32) ( tmp_u64 % 16LL ) ];
              tmp_u64 = ( tmp_u64 >> 4 );
              tmp_count++;
            }
            break;
          case 2:
            tmp_u32 = (u32) va_arg( args, u32 );

            for ( u = 0 ; u < 4 ; u++ )
            {
              *ptr_ascii++ = hex_table[ tmp_u32 & 0x0F ];
              tmp_u32 = tmp_u32 >> 4;
              tmp_count++;
            }
            break;
        }
        *ptr_ascii++ = 0;
        state = COSM_IO_TMP_STRING;
        break;
    }
  }
  return (u64) 0;
}

#undef _COSM_IO_CLEARTMP
#undef _COSM_IO_CHECKMAX
#undef _COSM_IO_FETCHCHAR

s32 Cosm_PrintChar( u32 * bytes, cosm_FILE * file, void * string, u32 max_bytes,
  utf8char character )
{
  utf8 * ptr;
  utf8 array[8];
  u32 length;
  u64 written;
  u32 i;

  if ( ( Cosm_EncodeUTF8( array, &length, character ) == COSM_FAIL )
    || ( length > max_bytes ) )
  {
    return COSM_FAIL;
  }

  if ( string != NULL )
  {
    ptr = (utf8 *) string;
    for ( i = 0 ; i < length ; i++ )
    {
      *ptr++ = array[i];
    }
  }
  else if ( file != NULL )
  {
    if ( COSM_PASS != CosmFileWrite( file, &written, array, length ) )
    {
      return COSM_FAIL;
    }
  }
  else /* standard output */
  {
    if ( fwrite( array, 1, length, stdout ) != length )
    {
      return COSM_FAIL;
    }
  }

  *bytes = length;

  return COSM_PASS;
}

/* testing */

#if 0 /* unused */
typedef struct cosm_FLOAT_TEST
{
  f64 data;
  const ascii * format;
  const ascii * result;
} cosm_FLOAT_TEST;

/* tests for %f, %F */
static cosm_FLOAT_TEST float_tests[] =
{
/*  { (f64) 1.0, "", "" }, */
  { (f64) 1.0, "%.1f", "1.0" },
  { (f64) -1.0, "%.1f", "-1.0" },
  { (f64) 0.0, NULL, NULL }
};

typedef struct cosm_SU32_TEST
{
  u32 data;
  const ascii * format;
  const ascii * result;
} cosm_SU32_TEST;

/* tests for %i, %u, %X */
static cosm_SU32_TEST su32_tests[] =
{
/*  { (u32) 0x00000000, "", "" }, */
  { (u32) 0x00000001, "%u", "1" },
  { (u32) 0x00000001, "%i", "1" },
  { (u32) 0xFFFFFFFF, "%u", "4294967295" },
  { (u32) 0xFFFFFFFF, "%i", "-1" },
  { (u32) 0x00000001, "%X", "1" },
  { (u32) 0x00000001, "%04X", "0001" },
  { (u32) 0xFFFFFFFF, "%X", "FFFFFFFF" },
  { (u32) 0.0,  NULL,  NULL }
};
#endif /* unused */

/* testing */

s32 Cosm_TestOSIO( void )
{
  ascii a_str1[1024] = "26 character test string.";
  ascii a_str2[512] = "";
  ascii a_str3[128] = "Append test string part 1";
  ascii a_str4[32] = "Append test part 2";
  u32 i;
  ascii * found;

  u32 tmpNum;
  ascii ascNum[6] = " 0xfF";
  /* " 0xfF" in several languages */
/*
  utf8 uniNum[6] = { 0x1680, '0', 'x', 0x0396, 0x05D5 , 0 };
*/
#ifdef F64TESTS
  f64 tmpf64;
#endif
  f32 tmpf32;
  s32 status;
  utf8 * pos;

  /* ASCII tests */

  if ( CosmStrBytes( NULL ) != 0 )
  {
    return -1;
  }

  if ( CosmStrBytes( a_str1 ) != 25 )
  {
    return -2;
  }

  /* CosmStrCopy, Copy a_str1 to a_str2 then compare. */
  if ( CosmStrCopy( a_str2, a_str1, 512 ) != COSM_PASS )
  {
    return -3;
  }

  for ( i = 0 ; i < 26 ; i++ )
  {
    if ( a_str1[i] != a_str2[i] )
    {
      return -4;
    }
  }

  /* Compare a_str1 with a_str2. a_str1 is the same as a_str2 as a result
     of the above CosmStrCopy test. Should return 0. */
  if ( CosmStrCmp( a_str1, a_str2, sizeof( a_str1 ) ) != 0 )
  {
    return -5;
  }

  a_str2[3] = 'b';

  /* Should return a positive number */
  if ( CosmStrCmp( a_str1, a_str2, sizeof( a_str1 ) ) <= 0 )
  {
    return -6;
  }

  /* Should return a negative number */
  if ( CosmStrCmp( a_str2, a_str1, sizeof( a_str1 ) ) >= 0 )
  {
    return -7;
  }

  /* Find character 'a' in a_str1 */
  if ( ( found = CosmStrChar( a_str1, 'a', sizeof( a_str1 ) ) )
    == NULL )
  {
    return -8;
  }

  /* Convert a string containing " 0xfF" to a number */
  tmpNum = 0;
  CosmU32Str( &tmpNum, NULL, ascNum, 0 );
  if ( tmpNum != 255 )
  {
    return -9;
  }
  /* utf8 tests */
/*
  tmpNum = 0;
  CosmU32Str( &tmpNum, NULL, uniNum, 0 );
  if ( tmpNum != 255 )
  {
    return -10;
  }
*/

#ifdef F64TESTS
  /*
    Tests for Cosmf64A.
    !!! These tests might be platform dependant.
    !!! Checking floats for equality is dangerous.
  */

  /* Trivial case. */
  status = Cosmf64Str( &tmpf64, NULL, "1" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 1 ) )
  {
    return -11;
  }

  /* An actual fraction. */
  status = Cosmf64Str( &tmpf64, &pos, "1.25" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 1.25 ) || ( *pos != '\0' ) )
  {
    return -12;
  }
  /* A slightly dirtier fraction. */
  status = Cosmf64Str( &tmpf64, &pos, " \t  1.5Cookie." );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 1.5 ) || ( *pos != 'C' ) )
  {
    return -13;
  }

  /* A basic case with an exponent. */
  status = Cosmf64Str( &tmpf64, NULL, "1E1" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 10 )  )
  {
    return -14;
  }

  /* Some more playing around with exponents. */
  status = Cosmf64Str( &tmpf64, &pos, "2.5E-1" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 0.25 ) || ( *pos != '\0' ) )
  {
    return -15;
  }

  status = Cosmf64Str( &tmpf64, &pos, "1.25E25." );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 1.25E25 ) || ( *pos != '.' ) )
  {
    return -16;
  }

  status = Cosmf64Str( &tmpf64, &pos, "7.923941E5." );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 7.923941E5 ) || ( *pos != '.' ) )
  {
    return -17;
  }

  /* Now we should try a few 'silly' strings. */
  status = Cosmf64Str( &tmpf64, &pos, ".E-" );
  if ( status != COSM_FAIL )
  {
    return -18;
  }

  status = Cosmf64Str( &tmpf64, &pos, ".0-" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 0 ) || ( *pos != '-' ) )
  {
    return -19;
  }

  status = Cosmf64Str( &tmpf64, &pos, ".E5Cookie" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 0 ) || ( *pos != 'C' ) )
  {
    return -20;
  }

  /* This is a valid float since all elements are optional. */
  status = Cosmf64Str( &tmpf64, &pos, "Plain Cookie." );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 0 ) || ( *pos != 'P' ) )
  {
    return -21;
  }


  /* Our last few tests will try to break Cosmf64Str */
  status = Cosmf64Str( &tmpf64, NULL, "1234582.24321E2" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 1234582.24321E2 ) )
  {
    return -22;
  }

  status = Cosmf64Str( &tmpf64, &pos, "3.E-3.3" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 3E-3 ) || ( *pos != '.' ) )
  {
    return -23;
  }

  status = Cosmf64Str( &tmpf64, &pos, "" );
  if ( ( status != COSM_PASS ) || ( tmpf64 != 0 ) || ( *pos != '\0' ) )
  {
    return -24;
  }
#endif

  /* Overflow experiments. */
/*
  status = Cosmf64Str( &tmpf64, &pos, "1212222222222222222222222E2134" );
  if ( ( status != COSM_FAIL ) || ( tmpf64 != 1212222222222222222222222E2134 )
       || ( *pos != '\0' ) )
  {
    return -25;
  }

  status = Cosmf64Str( &tmpf64, &pos,
    "-1212222222222222222222222E2134" );

  if ( ( status != COSM_FAIL ) || ( tmpf64 != -1212222222222222222222222E2134 )
       || ( *pos != '\0' ) )
  {
    return -26;
  }
*/

  /*
    Cosmf32Str tests
    !!! These tests might be platform dependant.
    !!! Checking floats for equality is dangerous.
  */
  status = Cosmf32Str( &tmpf32, NULL, "1" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 0.999 ) ||
       ( tmpf32 > 1.001 ) )
  {
    return -27;
  }

  /* An actual fraction. */
  status = Cosmf32Str( &tmpf32, &pos, "1.25" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 1.2499 ) ||
       ( tmpf32 > 1.2501 ) ||
       ( *pos != '\0' ) )
  {
    return -28;
  }
  /* A slightly dirtier fraction. */
  status = Cosmf32Str( &tmpf32, &pos, " \t  1.5Cookie." );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 1.499 ) ||
       ( tmpf32 > 1.501 ) ||
       ( *pos != 'C' ) )
  {
    return -29;
  }

  /* A basic case with an exponent. */
  status = Cosmf32Str( &tmpf32, NULL, "1E1" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 9.99 ) ||
       ( tmpf32 > 10.01 ) )
  {
    return -30;
  }

  /* Some more playing around with exponents. */
  status = Cosmf32Str( &tmpf32, &pos, "2.5E-1" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 0.2499 ) ||
       ( tmpf32 > 0.2501 ) ||
       ( *pos != '\0' ) )
  {
    return -31;
  }

  status = Cosmf32Str( &tmpf32, &pos, "1.25E25." );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 1.249999E25 ) ||
       ( tmpf32 > 1.250001E25 ) ||
       ( *pos != '.' ) )
  {
    return -32;
  }

  status = Cosmf32Str( &tmpf32, &pos, "7.923941E5." );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 7.923940E5 ) ||
       ( tmpf32 > 7.923942E5 ) ||
       ( *pos != '.' ) )
  {
    return -33;
  }

  /* Now we should try a few 'silly' strings. */
  status = Cosmf32Str( &tmpf32, &pos, ".E-" );
  if ( status != COSM_FAIL )
  {
    return -34;
  }

  status = Cosmf32Str( &tmpf32, &pos, ".0-" );
  if ( ( status != COSM_PASS ) || ( tmpf32 != 0 ) || ( *pos != '-' ) )
  {
    return -35;
  }

  status = Cosmf32Str( &tmpf32, &pos, ".E5Cookie" );
  if ( ( status != COSM_PASS ) || ( tmpf32 != 0 ) || ( *pos != 'C' ) )
  {
    return -36;
  }

  /* This is a valid float since all elements are optional. */
  status = Cosmf32Str( &tmpf32, &pos, "Plain Cookie." );
  if ( ( status != COSM_PASS ) || ( tmpf32 != 0 ) || ( *pos != 'P' ) )
  {
    return -37;
  }


  /* Our last few tests will try to break Cosmf32Str */
  status = Cosmf32Str( &tmpf32, NULL, "1234582.24321E2" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 1234582.23E2 ) ||
       ( tmpf32 > 1234582.25E2 )  )
  {
    return -38;
  }

  status = Cosmf32Str( &tmpf32, &pos, "3.E-3.3" );
  if ( ( status != COSM_PASS ) ||
       ( tmpf32 < 2.99999E-3 ) ||
       ( tmpf32 > 3.00001E-3 ) ||
       ( *pos != '.' ) )
  {
    return -39;
  }

  status = Cosmf32Str( &tmpf32, &pos, "" );
  if ( ( status != COSM_PASS ) || ( tmpf32 != 0 ) || ( *pos != '\0' ) )
  {
    return -40;
  }

  /* Overflow experiments. */
/*
  status = Cosmf32Str( &tmpf32, &pos, "1212222222222222222222222E2134" );
  if ( ( status != COSM_FAIL ) || ( tmpf32 != 1212222222222222222222222E2134 )
       || ( *pos != '\0' ) )
  {
    return -41;
  }

  status = Cosmf32Str( &tmpf32, &pos,
    "-1212222222222222222222222E2134" );

  if ( ( status != COSM_FAIL ) || ( tmpf32 != -1212222222222222222222222E2134 )
       || ( *pos != '\0' ) )
  {
    return -42;
  }
*/

  /* Test CosmStrAppendA */

  /* Check string size limits */
  if ( CosmStrAppend( a_str3, a_str4, 25 ) != COSM_FAIL )
  {
    return -43;
  }

  /* Check it didnt alter anything */
  if ( CosmStrCmp( a_str3, "Append test string part 1", 127 ) != 0 )
  {
    return -44;
  }

  /* Try an actuall append */
  if ( CosmStrAppend( a_str3, a_str4, 127 ) != COSM_PASS )
  {
    return -45;
  }

  /* Is the string the correct length */
  if ( CosmStrBytes( a_str3 ) == 44 )
  {
    return -46;
  }

  /* Is the string what we expect it to be */
  if ( CosmStrCmp( a_str3,
    "Append test string part 1Append test part 2", 127 ) != 0 )
  {
    return -47;
  }

  return COSM_PASS;
}
