/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Cosm Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

/*
  We put all error I/O here because it will significantly increase
  the size of the code. This code will not end up in libCosm.
  Ironicly, the only function we can't test (CosmPrint) is the only
  one we use here.
*/

#include "cosm/cosm.h"

struct align_test
{
  u64 a;
  u32 b;
  u16 c;
  u8  d[2];
  u128 e;
  u8  f[2];
  u16 g;
  u32 h;
  u64 i;
} align_test; /* 48 bytes */

void CatchINT( int arg )
{
  CosmSignalRegister( COSM_SIGNAL_INT, CatchINT );
  CosmPrint( "SIGINT - ouch!\n" );
}

void CatchTERM( int arg )
{
  CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM );
  CosmPrint( "SIGTERM - ouch!\n" );
}

void ChildINT( int arg )
{
  CosmSignalRegister( COSM_SIGNAL_INT, ChildINT );
  CosmPrint( "    Child caught SIGINT - ouch!\n" );
}

void ChildTERM( int arg )
{
  CosmSignalRegister( COSM_SIGNAL_TERM, ChildTERM );
  CosmPrint( "    Child caught SIGTERM - ouch!\n" );
}

void Child( void )
{
  /*
  cosm_SEMAPHORE sem;
  cosm_SEMAPHORE_NAME sem_name;
  cosm_SHARED_MEM mem;
  cosm_SHARED_MEM_NAME mem_name;
  cosm_FILE file;
  */

  CosmPrint( "    Hello from child process %v\n", CosmProcessID() );

  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, ChildINT ) != COSM_PASS ) ||
    ( CosmSignalRegister( COSM_SIGNAL_TERM, ChildTERM ) != COSM_PASS ) )
  {
    CosmPrint( "    Child Signal registration failed\n" );
    CosmProcessEnd( -1 );
  }

  /* sleep long enough for tests */
  CosmSleep( 10000000 );

  /*
    semaphores
    shared memory    
  */
  
  CosmProcessEnd( 0 );
}

void Parent( char * self )
{
  u64 child_id;
  /*
  cosm_SEMAPHORE sem;
  cosm_SEMAPHORE_NAME sem_name;
  cosm_SHARED_MEM mem;
  cosm_SHARED_MEM_NAME mem_name;
  cosm_FILE file;
  */
  
  CosmPrint( "Spawning - %.256s -child\n", self );
  if ( CosmProcessSpawn( &child_id, self, "-child", NULL ) != COSM_PASS )
  {
    CosmPrint( "Spawn failed\n" );
  }
  else
  {
    CosmPrint( "Spawned PID %v\n", child_id );
  }

  /*
    semaphores
    shared memory
    
  */

  /* signal tests */
  CosmSleep( 1000 );
  if ( CosmSignal( child_id, COSM_SIGNAL_PING ) != COSM_PASS )
  {
    CosmPrint( "\nSending child SIGPING failed (not started?)\n" );
  }
  else
  {
    CosmSleep( 500 );
    CosmPrint( "\nSending child SIGPING worked (exists)\n" );
  }

  if ( CosmSignal( child_id, COSM_SIGNAL_INT ) != COSM_PASS )
  {
    CosmPrint( "Sending child SIGINT failed\n" );
  }
  else
  {
    CosmSleep( 500 );
    CosmPrint( "Sending child SIGINT worked\n" );
  }

  if ( CosmSignal( child_id, COSM_SIGNAL_TERM ) != COSM_PASS )
  {
    CosmPrint( "Sending child SIGTERM failed\n" );
  }
  else
  {
    CosmSleep( 500 );
    CosmPrint( "Sending child SIGTERM worked\n" );
  }

  if ( CosmSignal( child_id, COSM_SIGNAL_KILL ) != COSM_PASS )
  {
    CosmPrint( "Sending child SIGKILL failed\n" );
  }
  else
  {
    CosmSleep( 500 );
    CosmPrint( "Sending child SIGKILL worked\n" );
  }

  if ( CosmSignal( child_id, COSM_SIGNAL_PING ) == COSM_PASS )
  {
    CosmPrint( "\nChild SIGPING failed, thinks child exists\n" );
  }
  else
  {
    CosmPrint( "\nChild SIGPING worked (it's dead Jim)\n" );
  }
}

int main( int argc, char * argv[] )
{
  s32 error;
  s32 error2;
  const ascii * os_types[] = COSM_OS_TYPES;
  const ascii * cpu_types[] = COSM_CPU_TYPES;
  const ascii * months[12] = COSM_TIME_MONTHS;
  const ascii * days[7] = COSM_TIME_DAYS;
  cosm_DYNAMIC_LIB dylib;
  s32 (*dl_function)( s32 );
  u64 memory;
  utf8 buffer[256];
  cosmtime mytime;
  cosm_TIME_UNITS myunits;
  u64 milli = COSM_TIME_MILLISECOND;
  u32 count, i;
  cosm_NET_ADDR addr[16];
  cosm_LOG log;
  cosm_FILE file;
  cosm_FILENAME filename;
  cosm_FILE_INFO info;
  utf8 * correct;
#if ( defined( MEM_LEAK_FIND ) )
  utf8 * incorrect;
#endif
  utf8 buf[512];
  u64 bytesread;
  u64 process_id;
  const u8 endian_bytes[16] =
  {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
  };
  u128 b_u128, e_u128;

  if ( ( argc == 2 )
    && ( CosmStrCmp( argv[1], "-child", (u64) 8 ) == 0 ) )
  {
    Child();
  }

  count = 0;
  while ( os_types[count] != NULL )
  {
    count++;
  }
  if ( count != ( COSM_OS_TYPE_MAX + 1 ) )
  {
    CosmPrint( "COSM_OS_TYPES are incorrect in cputypes.h\n" );
    CosmProcessEnd( -1 );
  }

  count = 0;
  while ( cpu_types[count] != NULL )
  {
    count++;
  }
  if ( count != ( COSM_CPU_TYPE_MAX + 1 ) )
  {
    CosmPrint( "COSM_CPU_TYPES are incorrect in cputypes.h\n" );
    CosmProcessEnd( -1 );
  }

  CosmPrint( "CPU = %.20s * ",
    cpu_types[( CPU_TYPE > COSM_CPU_TYPE_MAX ) ? 0 : CPU_TYPE] );

  if ( CosmCPUCount( &count ) == COSM_PASS )
  {
    CosmPrint( "%u", count );
  }
  else
  {
    CosmPrint( "%u+", count );
  }

#if ( defined( CPU_64BIT ) )
  CosmPrint( ", 64-Bit" );
#elif ( defined( CPU_64BIT ) )
  CosmPrint( ", 32-Bit w/compiler u64" );
#else
  CosmPrint( ", 32-Bit" );
#endif

  CosmU128Load( &b_u128, endian_bytes );
  _COSM_SET128( e_u128, 0102030405060708, 090A0B0C0D0E0F10 );
  if ( !CosmU128Eq( b_u128, e_u128 ) )
  {
    CosmPrint( "\nCPU endian is incorrect set as %.6s in cputypes.h\n",
      ( COSM_ENDIAN_CURRENT == COSM_ENDIAN_BIG ) ? "BIG" : "LITTLE" );
    CosmProcessEnd( -1 );
  }

  CosmPrint( ", %.6s Endian",
    ( COSM_ENDIAN_CURRENT == COSM_ENDIAN_BIG ) ? "Big" : "Little" );

  CosmPrint( ", OS = %.20s\n",
    os_types[( OS_TYPE > COSM_OS_TYPE_MAX ) ? 0 : OS_TYPE] );


  if ( sizeof( align_test ) != 48 )
  {
    CosmPrint( "Data alignment is wrong, check the compiler.\n" );
    CosmProcessEnd( -1 );
  }

  if ( ( sizeof( u8 ) != 1 ) || ( sizeof( u16 ) != 2 )
    || ( sizeof( u32 ) != 4 ) || ( sizeof( u64 ) != 8 )
    || ( sizeof( u128 ) != 16 )
#if ( !defined( NO_FLOATING_POINT ) )
    || ( sizeof( f32 ) != 4 ) || ( sizeof( f64 ) != 8 )
#endif
    )
  {
    CosmPrint(
     "The data type sizes wrong! %u/1 %u/2 %u/4 %u/8 %u/16 %u/4 %u/8\n",
      sizeof( u8 ), sizeof( u16 ), sizeof( u32 ), sizeof( u64 ),
      sizeof( u128 ),
#if ( !defined( NO_FLOATING_POINT ) )
      sizeof( f32 ), sizeof( f64 )
#else
      4, 8
#endif
       );
    CosmProcessEnd( -1 );
  }

#if ( !defined( NO_FLOATING_POINT ) )
  CosmPrint( "Floating point support enabled.\n" );
  CosmPrint( "f32  %.20f (~7 significant digits)\n",
    (f32) 0.1234567890123456789012345678901234567890 );
  CosmPrint( "f64  %.20f (~16 significant digits)\n",
    (f64) 0.1234567890123456789012345678901234567890 );
#else
  CosmPrint( "Floating point support disabled.\n" );
#endif

  if ( CosmMemSystem( &memory ) == COSM_PASS )
  {
    CosmPrint( "\nMemory detected: %v bytes (~%v MiB)\n", memory,
      ( memory >> 20 ) );
  }
  else
  {
    CosmPrint( "\nMemory detected: UNKNOWN\n" );
  }

  if ( CosmDirGet( &filename ) == COSM_PASS )
  {
    CosmPrint( "Current directory = \"%.256s\"\n", filename );
  }
  else
  {
    CosmPrint( "Current directory = UNKNOWN\n" );
  }

  if ( ( count = CosmNetMyIP( addr, 16 ) ) > 0 )
  {
    for ( i = 0 ; i < count ; i++ )
    {
      if ( addr[i].type == COSM_NET_IPV4 )
      {
        CosmPrint( "IP = %u.%u.%u.%u\n",
          ( addr[i].ip.v4 >> 24 ) & 0xFF,
          ( addr[i].ip.v4 >> 16 ) & 0xFF,
          ( addr[i].ip.v4 >> 8 ) & 0xFF,
          addr[i].ip.v4 & 0xFF );
      }
      else
      {
        CosmPrint( "IP = %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X\n",
          (u32) ( addr[i].ip.v6.hi >> 48 ) & 0xFFFF,
          (u32) ( addr[i].ip.v6.hi >> 32 ) & 0xFFFF,
          (u32) ( addr[i].ip.v6.hi >> 16 ) & 0xFFFF,
          (u32) addr[i].ip.v6.hi & 0xFFFF,
          (u32) ( addr[i].ip.v6.lo >> 48 ) & 0xFFFF,
          (u32) ( addr[i].ip.v6.lo >> 32 ) & 0xFFFF,
          (u32) ( addr[i].ip.v6.lo >> 16 ) & 0xFFFF,
          (u32) addr[i].ip.v6.lo & 0xFFFF );
      }
    }
  }
  else
  {
    CosmPrint( "IP = UNKNOWN\n" );
  }

  CosmPrint( "\nRunning system tests... " );
  if ( CosmTest( &error, &error2, 0 ) != COSM_PASS )
  {
    CosmPrint( "Test failure in module %.16s %i.\n",
      __cosm_test_modules[-error].name, error2 );
    CosmProcessEnd( error );
  }
  CosmPrint( "all passed.\n" );

  CosmPrint( "Dynamic Library test... " );
  CosmMemSet( &dylib, sizeof( cosm_DYNAMIC_LIB ), 0 );
  if ( COSM_PASS != CosmDynamicLibLoad( &dylib, "./test_dl.dylib" ) )
  {
    CosmPrint( "Failed to load dynamic library.\n" );
  }
  else
  {
    if ( ( dl_function = CosmDynamicLibGet( &dylib,
      "TestDynamicLib" ) ) == NULL )
    {
      CosmPrint( "Failed to load TestDynamicLib.\n" );
    }
    else
    {
      CosmPrint( "f(x) = x^2. f(%i) = %i.\n", 3, (*dl_function)( 3 ) );
    }
    CosmDynamicLibFree( &dylib );
  }
  
  CosmPrint( "\nRun ANSI color test? [y/N] " );
  CosmInput( buffer, 4, COSM_IO_ECHO );
  if ( ( buffer[0] == 'y' ) || ( buffer[0] == 'Y' ) )
  {
    CosmPrint( "%.5s%.5sS%.5s%.5sP%.5s%.5sE%.5s%.5sC",
      ANSI_BG_WHITE, ANSI_FG_BLACK, ANSI_BG_MAGENTA, ANSI_FG_RED,
      ANSI_BG_BLUE, ANSI_FG_YELLOW, ANSI_BG_CYAN, ANSI_FG_GREEN );
    CosmPrint( "%.5s%.5sT%.5s%.5sR%.5s%.5sU%.5s%.5sM",
      ANSI_BG_GREEN, ANSI_FG_CYAN, ANSI_BG_YELLOW, ANSI_FG_BLUE,
      ANSI_BG_RED, ANSI_FG_MAGENTA, ANSI_BG_BLACK, ANSI_FG_WHITE );
    CosmPrint( "%.5s\n\n", ANSI_CLEAR );
  }

  CosmPrint(
   "Type 20 characters, 15 will be read (should be echoed): " );
  CosmInput( buffer, 16, COSM_IO_ECHO );
  CosmPrint( "You typed \"%.255s\"\n", buffer );
  CosmPrint(
   "Type 20 characters, 15 will be read (should not be echoed): " );
  CosmInput( buffer, 16, COSM_IO_NOECHO );
  CosmPrint( "You typed \"%.255s\"\n", buffer );

  CosmPrint(
   "\nThe following 6 times should be about 1/2 second apart:\n" );
  for ( count = 0 ; count < 6 ; count++ )
  {
    if ( CosmSystemClock( &mytime ) == COSM_PASS )
    {
      if ( CosmTimeUnitsGregorian( &myunits, mytime ) == COSM_PASS )
      {
        CosmPrint( "%.9s %.9s %u, %j %02u:%02u:%02u.%03v UTC, d%u\n",
          days[myunits.wday], months[myunits.month], myunits.day + 1,
          myunits.year, myunits.hour, myunits.min, myunits.sec,
          ( myunits.subsec / milli ), myunits.yday + 1 );
        CosmSleep( 500 );
      }
    }
  }

  /* test the log echo's */
  CosmMemSet( &log, sizeof( cosm_LOG ), 0 );
  CosmMemSet( &file, sizeof( cosm_FILE ), 0 );
  if ( CosmLogOpen( &log, "testlib.log", 5, COSM_LOG_MODE_NUMBER )
    != COSM_PASS )
  {
    CosmPrint( "Unable to open test log.\n" );
    CosmProcessEnd( -1 );
  }
  else
  {
    CosmPrint( "\nThis should be logged and echoed: " );
    CosmLog( &log, 3, COSM_LOG_ECHO, "Logged and echoed.\n" );
    CosmPrint( "This should be logged and NOT echoed: " );
    CosmLog( &log, 3, COSM_LOG_NOECHO, "Logged and not echoed.\n" );
    CosmPrint( "\n" );
    CosmLogClose( &log );

    correct = "Logged and echoed.\nLogged and not echoed.\n";
    if ( CosmFileOpen( &file, "testlib.log", COSM_FILE_MODE_READ,
      COSM_FILE_LOCK_NONE ) != COSM_PASS )
    {
      CosmPrint( "Unable to open test log for checking.\n" );
    }
    else
    {
      CosmFileRead( buf, &bytesread, &file, (u64) CosmStrBytes( correct ) );

      if ( CosmStrCmp( buf, correct, CosmStrBytes( correct ) ) != COSM_PASS )
      {
        CosmPrint( "Error in logging/echo.\n" );
      }
      else
      {
        CosmPrint( "Logging/echo correct.\n" );
      }
      CosmFileClose( &file );
    }
  }

  CosmFileDelete( "testlib.log" );

  /* test fileinfo */
  CosmFileInfo( &info, argv[0] );
  CosmPrint( "\ntestlib.exe info:\nLength: %v\n", info.length );
  CosmPrint( "Type: %.10s\n",
    ( info.type == COSM_FILE_TYPE_FILE ) ? "File" :
    ( info.type == COSM_FILE_TYPE_DIR ) ? "Directory" :
    ( info.type == COSM_FILE_TYPE_DEVICE ) ? "Device" :
    ( info.type == COSM_FILE_TYPE_SPECIAL ) ? "Special" :
    "UNKNOWN" );
  CosmPrint( "Rights:" );
  if ( info.rights & COSM_FILE_RIGHTS_READ )
  {
    CosmPrint( " Read" );
  }
  if ( info.rights & COSM_FILE_RIGHTS_WRITE )
  {
    CosmPrint( " Write" );
  }
  if ( info.rights & COSM_FILE_RIGHTS_APPEND )
  {
    CosmPrint( " Append" );
  }
  if ( info.rights & COSM_FILE_RIGHTS_EXEC )
  {
    CosmPrint( " Execute" );
  }
  CosmPrint( "\n" );

  if ( CosmTimeUnitsGregorian( &myunits, info.create ) == COSM_PASS )
  {
    CosmPrint( "Create: %.9s %.9s %u, %j %02u:%02u:%02u.%03v UTC, d%u\n",
      days[myunits.wday], months[myunits.month], myunits.day + 1,
      myunits.year, myunits.hour, myunits.min, myunits.sec,
      ( myunits.subsec / milli ), myunits.yday + 1 );
  }
  if ( CosmTimeUnitsGregorian( &myunits, info.modify ) == COSM_PASS )
  {
    CosmPrint( "Modify: %.9s %.9s %u, %j %02u:%02u:%02u.%03v UTC, d%u\n",
      days[myunits.wday], months[myunits.month], myunits.day + 1,
      myunits.year, myunits.hour, myunits.min, myunits.sec,
      ( myunits.subsec / milli ), myunits.yday + 1 );
  }
  if ( CosmTimeUnitsGregorian( &myunits, info.access ) == COSM_PASS )
  {
    CosmPrint( "Access: %.9s %.9s %u, %j %02u:%02u:%02u.%03v UTC, d%u\n",
      days[myunits.wday], months[myunits.month], myunits.day + 1,
      myunits.year, myunits.hour, myunits.min, myunits.sec,
      ( myunits.subsec / milli ), myunits.yday + 1 );
  }

#if ( defined( MEM_LEAK_FIND ) )
  correct = CosmMemAlloc( 37LL );
  correct = CosmMemRealloc( correct, 73LL );
  incorrect = CosmMemAlloc( 13LL );
  incorrect[15] = 0; /* <- this is a buffer overflow */
  CosmPrint( "\nleaks.txt should contain one 73 byte leak at %p,\n"
    "  and a corrupted 13 byte leak at %p\n",
    correct, incorrect );
  CosmMemDumpLeaks( "leaks.txt" );
  CosmMemFree( correct );
  CosmMemFree( incorrect );
#else
  CosmPrint( "\nDefine MEM_LEAK_FIND to test CosmMemDumpLeaks()\n" );
#endif

  CosmPrint( "\nRun process spawning test? [y/N] " );
  CosmInput( buffer, 16, COSM_IO_ECHO );
  if ( ( buffer[0] == 'y' ) || ( buffer[0] == 'Y' ) )
  {
    Parent( argv[0] );
  }

  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, CatchINT ) != COSM_PASS ) ||
    ( CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM ) != COSM_PASS ) )
  {
    CosmPrint( "Signal registration failed\n" );
    CosmProcessEnd( -1 );
  }

  process_id = CosmProcessID();
  if ( CosmSignal( process_id, COSM_SIGNAL_PING ) != COSM_PASS )
  {
    CosmPrint( "\nSelf SIGPING failed\n" );
    CosmProcessEnd( -1 );
  }
  else
  {
    CosmPrint( "\nSelf SIGPING worked\n" );
  }

  CosmSleep( 500 );
  CosmPrint( "\nSending self SIGINT\n" );
  if ( CosmSignal( process_id, COSM_SIGNAL_INT ) != COSM_PASS )
  {
    CosmPrint( "Self SIGINT failed\n" );
  }
  else
  {
    CosmPrint( "Self SIGINT worked\n" );
  }

  CosmSleep( 500 );
  CosmPrint( "\nSending self SIGTERM\n" );
  if ( CosmSignal( process_id, COSM_SIGNAL_TERM ) != COSM_PASS )
  {
    CosmPrint( "Self SIGTERM failed\n" );
  }
  else
  {
    CosmPrint( "Self SIGTERM worked\n" );
  }

  CosmProcessEnd( 0 );
  return 0;
}
