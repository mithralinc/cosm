/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm CS-SDK2 - Client Server Software Development Kit

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  2009-2012 by Creator. All rights reserved. Further information about the
  Package can be found at the Creator's web site:
  http://www.mithral.com/
*/

/*
  Threads:

  Tracker (300)
    - Accept check-in/out requests.
    - Reply with IP and record.

  Pinger
    - Generates pings to hosts we think are still alive.
    - Accept pongs, timestamp immediately then record.

Restart
  Send pings to everyone that isnt shutdown.
Shutdown
*/

#include "cssdk.h"
#include "PostgreSQL.h"

void Notice( void );
void Usage( void );
void CatchINT( int arg );
void CatchTERM( int arg );
void Tracker( void * arg );
void Pinger( void * arg );

u32 __shutdown_flag = 0;   /* interupt flag */
u32 __tracker_alive = 0;
u32 __pinger_alive = 0;
cosm_LOG __server_log;
u32 __seq = 0;

void Notice( void )
{
  cosmtime mytime;
  cosm_TIME_UNITS myunits;

  CosmSystemClock( &mytime );
  CosmTimeUnitsGregorian( &myunits, mytime );

  CosmPrint(
    "Cosm CS-SDK2 Server version %u.%u.%u\n"
    "Copyright Mithral Communications & Design, Inc. 2009-%i.\n"
    "Visit http://www.mithral.com/ for more information.\n\n",
    CSSDK_VER_MAJOR, CSSDK_VER_MINOR_MIN, CSSDK_VER_MINOR_MAX,
    myunits.year );
}

void Usage( void )
{
  CosmPrint( "usage: cssdk-server.exe log_level\n"
    "  0 = only critical\n"
    "  3 = + checkins and pongs\n"
    "  6 = + pings\n"
    "  9 = + full SQL\n"
    "\n\n" );
}

void CatchINT( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_INT, CatchINT );
}

void CatchTERM( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM );
}

void DumpQuery( DATABASE * db, utf8 * query )
{
  u32 i, j, x, y;
  utf8 * str;

  if ( ( SQL_Exec( db, query ) != COSM_PASS )
    || ( SQL_Rows( &x, db ) != COSM_PASS )
    || ( SQL_Columns( &y, db ) != COSM_PASS ) )
  {
    CosmPrint( "Database connection error\n" );
    return;
  }

  CosmPrint( "%.999s\n", query );

  for ( i = 0 ; i < x ; i++ )
  {
    for ( j = 0 ; j < y ; j++ )
    {
      SQL_GetValue( &str, db, i, j );
      CosmPrint( "%u, %u = ( '%.128s' )\n", i, j,
        ( str == NULL ) ? "NULL" : str );
    }
  }
}

void Tracker( void * arg )
{
  DATABASE database;
  cosm_NET_ADDR server, from;
  cosm_NET net;
  packet pkt;
  u32 recv_bytes;
  cosmtime time_now;
  ascii query[128];
  ascii * result;
  s64 ms;

  __tracker_alive = 1;

  if ( SQL_Connect( &database ) != COSM_PASS )
  {
    CosmLog( &__server_log, 0, COSM_LOG_ECHO,
      "%.32sUnable to connect to database (Tracker)\n", Now() );
    __shutdown_flag = 1;
    __tracker_alive = 0;
    return;
  }

  /* listening address */
  if ( CosmNetDNS( &server, 1, CSSDK_SERVER_SELF ) < 1 )
  {
    CosmLog( &__server_log, 0, COSM_LOG_ECHO,
      "%.32sUnable to resolve %.128s\n", Now(), CSSDK_SERVER_SELF );
    __shutdown_flag = 1;
    __tracker_alive = 0;
    return;
  }
  server.port = CSSDK_PORT_SERVER;

  CosmMemSet( &net, sizeof( net ), 0 );
  if ( CosmNetListen( &net, &server, COSM_NET_MODE_UDP, 8 ) != COSM_PASS )
  {
    CosmLog( &__server_log, 0, COSM_LOG_ECHO,
      "%.32sUnable to open listening socket\n", Now() );
    SQL_Close( &database );
    __shutdown_flag = 1;
    __tracker_alive = 0;
    return;
  }

  while ( __shutdown_flag == 0 )
  {
    if ( ( CosmNetRecvUDP( &pkt, &recv_bytes, &from, &net, sizeof( pkt ),
      NULL, 100 ) == COSM_PASS ) && ( recv_bytes == sizeof( pkt ) ) )
    {
      CosmSystemClock( &time_now );
      PacketDecode( &pkt );

      if ( pkt.signature != CSSDK_SIGNATURE )
      {
        /* bad packet */
        continue;
      }
      
      if ( ( pkt.ver_major < CSSDK_VER_MAJOR )
        || ( pkt.ver_minor_min < CSSDK_VER_MINOR_MIN ) )
      {
        /* out of data client, let them know what's current */
        /* !!! send a version only packet */        
        continue;
      }
      
      switch ( pkt.type )
      {
        case TYPE_PONG:
          /* record a pong */
          ms = CosmS64S128( CosmS128Div( CosmS128Sub( time_now,
            pkt.timestamp ), CosmS128S64( COSM_TIME_MILLISECOND ) ) );
          CosmPrintStr( query, 128,
            "SELECT pong( x'%X'::int, x'%X'::int, %j, %j );",
            pkt.id, from.ip.v4, time_now.hi, ms );

          CosmLog( &__server_log, 9, COSM_LOG_ECHO, "%.999s\n", query );
          if ( SQL_Exec( &database, query ) != COSM_PASS )
          {
            CosmLog( &__server_log, 0, COSM_LOG_ECHO,
              "%.32sDatabase error 1\n", Now() );
            goto bailout;
          }
          
          CosmLog( &__server_log, 3, COSM_LOG_ECHO,
            "%.32spong:%08X ip:%u.%u.%u.%u seq:%u ms:%j\n",
            Now(), pkt.id,
            ( from.ip.v4 >> 24 ) & 0xFF, ( from.ip.v4 >> 16 ) & 0xFF,
            ( from.ip.v4 >> 8 ) & 0xFF, from.ip.v4 & 0xFF, pkt.seq, ms );
          break;

        case TYPE_RESTART:
        case TYPE_CHECKIN:
        case TYPE_SHUTDOWN:
          /* various types of checkin */
          CosmPrintStr( query, 128,
            "SELECT checkin( x'%X'::int, %u, %u, %u, x'%X'::int, "
            "'%u.%u.%u.%u', %j, '%c', %j );",
            pkt.id, pkt.os, pkt.cpu, pkt.mem, from.ip.v4,
            ( from.ip.v4 >> 24 ) & 0xFF, ( from.ip.v4 >> 16 ) & 0xFF,
            ( from.ip.v4 >> 8 ) & 0xFF, from.ip.v4 & 0xFF,
            time_now.hi, pkt.type, pkt.timestamp.hi );

          CosmLog( &__server_log, 9, COSM_LOG_ECHO, "%.999s\n", query );
          if ( ( SQL_Exec( &database, query ) != COSM_PASS )
            || ( SQL_GetValue( &result, &database, 0, 0 )
            != COSM_PASS ) || ( result == NULL ) )
          {
            CosmLog( &__server_log, 0, COSM_LOG_ECHO,
              "%.32sDatabase error 2\n", Now() );
            goto bailout;
          }

          CosmLog( &__server_log, 3, COSM_LOG_ECHO,
            "%.32scheckin:%08X ip:%u.%u.%u.%u port:%u "
            "os:%u cpu:%u mem:%u addr:%X seq:%u found:%c type:%c\n", Now(),
            pkt.id, ( from.ip.v4 >> 24 ) & 0xFF, ( from.ip.v4 >> 16 ) & 0xFF,
            ( from.ip.v4 >> 8 ) & 0xFF, from.ip.v4 & 0xFF, from.port,
            (u32) pkt.os, (u32) pkt.cpu, pkt.mem, pkt.address, pkt.seq,
            result[0], pkt.type );
          break;

        default:
          break;          
      }
    }
  }
bailout:
  CosmNetClose( &net );
  SQL_Close( &database );
  __tracker_alive = 0;
}

void Pinger( void * arg )
{
  DATABASE database;
  cosm_NET_ADDR to;
  cosm_NET net;
  packet ping;
  cosmtime time_now;
  ascii query[1024];
  u32 i, rows;
  utf8 * str;
  u32 id;
  u32 addr;

  __pinger_alive = 1;

  if ( SQL_Connect( &database ) != COSM_PASS )
  {
    CosmLog( &__server_log, 0, COSM_LOG_ECHO,
      "%.32sUnable to connect to database (Pinger)\n", Now() );
    __shutdown_flag = 1;
    __pinger_alive = 0;
    return;
  }

  while ( __shutdown_flag == 0 )
  {
    CosmSystemClock( &time_now );

    CosmPrintStr( query, 1024,
      "SELECT * FROM live_hosts( %j );", time_now.hi );
    CosmLog( &__server_log, 9, COSM_LOG_ECHO, "%.999s\n", query );
    if ( ( SQL_Exec( &database, query ) != COSM_PASS )
      || ( SQL_Rows( &rows, &database ) != COSM_PASS ) )
    {
      CosmLog( &__server_log, 0, COSM_LOG_ECHO,
        "%.32sDatabase error 3\n", Now() );
      __shutdown_flag = 1;
      __pinger_alive = 0;
      return;
    }

    /* connect */
    CosmMemSet( &net, sizeof( net ), 0 );
    to.type = COSM_NET_IPV4;
    to.ip.v4 = 0;
    to.port = 0;

    CosmNetOpen( &net, NULL, &to, COSM_NET_MODE_UDP );

    for ( i = 0 ; i < rows ; i++ )
    {
      SQL_GetValue( &str, &database, i, 0 );
      CosmU32Str( &id, NULL, str, 16 );
      SQL_GetValue( &str, &database, i, 1 );
      CosmU32Str( &addr, NULL, str, 16 );

      /* Prepare a ping w/timestamp */
      ping.signature = CSSDK_SIGNATURE;
      ping.ver_major = CSSDK_VER_MAJOR;
      ping.ver_minor_min = CSSDK_VER_MINOR_MAX;
      ping.ver_minor_max = CSSDK_VER_MINOR_MAX;
      ping.os = 0;
      ping.cpu = 0;
      ping.id = id;
      ping.address = addr;
      ping.seq = __seq++;
      CosmSystemClock( &ping.timestamp );
      ping.type = TYPE_PING;
      
      PacketEncode( &ping );
      
      to.type = COSM_NET_IPV4;
      to.ip.v4 = addr;
      to.port = CSSDK_PORT_CLIENT;

      CosmNetSendUDP( &net, &to, &ping, sizeof( ping ) );
      CosmLog( &__server_log, 6, COSM_LOG_ECHO, "%.32sping:%08X\n",
        Now(), id );
    }

    CosmNetClose( &net );

    /* rest for CSSDK_DB_REST seconds before we check the DB again */
    for ( i = 0 ; ( ( i < CSSDK_DB_REST )
      && ( __shutdown_flag == 0 ) ) ; i++ )
    {
      CosmSleep( 1000 );
    }
  }
  SQL_Close( &database );
  __pinger_alive = 0;
}

int main( int argc, char *argv[] )
{
  u64 thread_tracker, thread_pinger;
  u32 log_lvl;

  Notice();

  if ( argc != 2 )
  {
    Usage();
    CosmProcessEnd( -1 );
  }

  if ( CosmU32Str( &log_lvl, NULL, argv[1], 10 ) != COSM_PASS )
  {
    Usage();
    CosmProcessEnd( -2 );  
  }

  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, CatchINT ) != COSM_PASS ) ||
    ( CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM ) != COSM_PASS ) )
  {
    CosmProcessEnd( -3 );
  }

  /* open __server_log */
  CosmMemSet( &__server_log, sizeof( __server_log ), 0 );
  if ( CosmLogOpen( &__server_log, "cssdk-server.log", log_lvl,
    COSM_LOG_MODE_NUMBER ) != COSM_PASS )
  {
    CosmPrint( "Unable to open log\n" );
    CosmProcessEnd( -4 );
  }

  CosmThreadBegin( &thread_tracker, Tracker, NULL, 64*1024 );
  /* let the Tracker get a headstart */
  CosmSleep( 2000 );
  CosmThreadBegin( &thread_pinger, Pinger, NULL, 64*1024 );

  CosmLog( &__server_log, 0, COSM_LOG_ECHO, "%.32sRunning\n", Now() );

  while  ( ( __shutdown_flag == 0 )
    || ( __tracker_alive == 1 )
    || ( __pinger_alive == 1 ) )
  {
    CosmSleep( 1000 );
  }

  /* now we can shutdown */
  CosmLog( &__server_log, 0, COSM_LOG_ECHO, "%.32sShutdown\n\n", Now() );
  CosmLogClose( &__server_log );

  return COSM_PASS;
}
