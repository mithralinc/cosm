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

/* this is a dynamic library */
#define USE_EXPORTS

#include "cssdk.h"

static void ReadLastShutdown( void );
static s32 WriteLastShutdown( s64 time );
static void Checkin( u32 type );
static void Tracker( void * arg );
static void Pinger( void * arg );

static u32 __tracker_alive = 0;
static u32 __pinger_alive = 0;
static u32 __expired_client = 0;
static u32 __expired_engine = 0;

static u32 __machine_id = 0;
static u32 __my_address = 0;

static s64 __last_checkin = 0;
static s64 __last_ping = 0;
static s64 __last_shutdown = 0;
static s64 __startup = 0;
static cosmtime __expire;

static u32 __seq = 0;
static cosm_LOG * __mylog = NULL;
static u32 * __shutdown_flag = NULL;

static void ReadLastShutdown( void )
{
  cosm_FILE file;
  u64 bytes_read;

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, "cssdk-client.time", COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_NONE ) != COSM_PASS )
  {
    /* no file */
    __last_shutdown = 0;
    return;
  }

  if ( CosmFileRead( &__last_shutdown, &bytes_read,
    &file, sizeof( __last_shutdown ) ) != COSM_PASS )
  {
    CosmFileClose( &file );
    __last_shutdown = 0;
    return;
  }

  CosmFileClose( &file );
}

static s32 WriteLastShutdown( s64 time )
{
  cosm_FILE file;
  u64 bytes_written;

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, "cssdk-client.time",
    COSM_FILE_MODE_CREATE | COSM_FILE_MODE_WRITE,
    COSM_FILE_LOCK_NONE ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  if ( CosmFileWrite( &file, &bytes_written, &time, sizeof( time ) )
    != COSM_PASS )
  {
    CosmFileClose( &file );
    return COSM_FAIL;
  }

  CosmFileClose( &file );
  return COSM_PASS;
}

static void Checkin( u32 type )
{
  cosm_NET_ADDR server;
  cosm_NET net;
  cosmtime now;
  packet pkt;
  u64 raw_mem;

  CosmSystemClock( &now );

  pkt.signature = CSSDK_SIGNATURE;
  pkt.ver_major = CSSDK_VER_MAJOR;
  pkt.ver_minor_min = CSSDK_VER_MINOR_MAX;
  pkt.ver_minor_max = CSSDK_VER_MINOR_MAX;
  pkt.os = OS_TYPE;
  pkt.cpu = CPU_TYPE;
  CosmMemSystem( &raw_mem );
  pkt.mem = (u32) ( raw_mem / 1024LL / 1024LL );
  pkt.id = __machine_id;
  pkt.address = __my_address;
  pkt.seq = __seq++;
  switch( type )
  {
    case TYPE_RESTART:
      pkt.timestamp.hi =
        ( __last_shutdown == -1 ) ? -1 : now.hi - __last_shutdown;
      break;
    case TYPE_CHECKIN:
      pkt.timestamp.hi =
        ( __last_ping == 0 ) ? -1 : now.hi - __last_ping;
      break;
    case TYPE_SHUTDOWN:
      pkt.timestamp.hi = now.hi - __startup;
      break;
  }
  pkt.timestamp.lo = 0;
  pkt.type = type;

  CosmLog( __mylog, 3, COSM_LOG_ECHO,
    "%.32sCheckin: id:%X, os:%u, cpu:%u, mem:%u addr:%u.%u.%u.%u, type:%c\n",
    Now(), pkt.id, (u32) pkt.os, (u32) pkt.cpu, pkt.mem,
    ( pkt.address >> 24 ) & 0xFF, ( pkt.address >> 16 ) & 0xFF,
    ( pkt.address >> 8 ) & 0xFF, pkt.address & 0xFF, pkt.type );

  PacketEncode( &pkt );

  /* server's address */
  if ( CosmNetDNS( &server, 1, CSSDK_SERVER ) < 1 )
  {
    CosmLog( __mylog, 0, COSM_LOG_ECHO,
      "%.32sUnable to resolve %.128s\n", Now(), CSSDK_SERVER );
    return;
  }
  server.port = CSSDK_PORT_SERVER;

  /* connect */
  CosmMemSet( &net, sizeof( net ), 0 );
  CosmNetOpen( &net, NULL, &server, COSM_NET_MODE_UDP );

  /* send and cross fingers - it's UDP! */
  CosmNetSendUDP( &net, &server, &pkt, sizeof( pkt ) );
  CosmNetClose( &net );

  __last_checkin = now.hi;
}

static void Tracker( void * arg )
{
  cosmtime now;

  __tracker_alive = 1;

  /* check-in for startup */
  Checkin( TYPE_RESTART );

  /* monitor that we hear from server, until shutdown */
  while ( *__shutdown_flag == 0 )
  {
    CosmSleep( 1000 );
    CosmSystemClock( &now );

    /* ratelimit checkin to every X seconds */
    if ( ( now.hi - __last_checkin ) > CSSDK_CHECKIN_RATE )
    {
      Checkin( TYPE_CHECKIN );
    }

    if ( CosmS128Gt( now, __expire ) )
    {
      /* we've expired */
      CosmLog( __mylog, 0, COSM_LOG_ECHO,
        "%.32sThis program has expired. "
        "Visit %.128s for updates or information.\n\n",
        Now(), CSSDK_STATS );
      *__shutdown_flag = 1;
    }
  }

  /* check-out ASAP for shutdown */
  Checkin( TYPE_SHUTDOWN );

  __tracker_alive = 0;
}

static void Pinger( void * arg )
{
  cosm_NET_ADDR from, server;
  cosm_NET * net;
  cosm_NET_ACL acl;
  packet pkt;
  u32 recv_bytes;
  s32 error;
  u32 address;
  u32 seq;
  u8 type;
  cosmtime now;
  ascii * now_str;

  net = (cosm_NET *) arg;
   
  /* server's address */
  if ( CosmNetDNS( &server, 1, CSSDK_SERVER ) < 1 )
  {
    CosmLog( __mylog, 0, COSM_LOG_ECHO,
      "%.32sUnable to resolve %.128s\n", Now(), CSSDK_SERVER );
    return;
  }
  server.port = CSSDK_PORT_SERVER;

  /* ACL - only allow UDP packets from the server */
  CosmSystemClock( &now );
  now.hi += 0xFFFFFFFF; /* ~forever */
  CosmMemSet( &acl, sizeof( acl ), 0 );
  /* deny everything */
  CosmNetACLAdd( &acl, &server, 0, COSM_NET_DENY, now );
  /* allow server */
  CosmNetACLAdd( &acl, &server, 32, COSM_NET_ALLOW, now );

  __pinger_alive = 1;

  while ( *__shutdown_flag == 0 )
  {
    if ( ( error = CosmNetRecvUDP( &pkt, &recv_bytes, &from, net,
      sizeof( pkt ), &acl, 1000 ) ) != COSM_PASS )
    {
      /* lost our socket */
      CosmLog( __mylog, 0, COSM_LOG_ECHO,
        "Listening socket lost error=%i\n", error );
      *__shutdown_flag = 1;
      __pinger_alive = 0;
      return;
    }
    PacketDecode( &pkt );

    /* bad packet or too old */
    if ( ( recv_bytes != sizeof( pkt ) )
      || ( pkt.signature != CSSDK_SIGNATURE )
      || ( pkt.type != TYPE_PING ) )
    {
      continue;
    }

    /* old engine */
    if ( ( pkt.ver_major > CSSDK_VER_MAJOR )
      || ( pkt.ver_minor_min > CSSDK_VER_MINOR ) )
    {
      __expired_engine = 1;
      break;
    }

    /* not for me */
    if ( pkt.id != __machine_id )
    {
      CosmLog( __mylog, 1, COSM_LOG_ECHO,
        "%.32sPing for %08X not %08X, bad port forwarding?\n",
        Now(), pkt.id, __machine_id );
      continue;
    }
    
    address = pkt.address;
    seq = pkt.seq;
    type = pkt.type;

    /* reply ASAP, only to server */
    pkt.type = TYPE_PONG;
    PacketEncode( &pkt );
    CosmNetSendUDP( net, &server, &pkt, sizeof( pkt ) );

    now_str = Now();
    CosmLog( __mylog, 3, COSM_LOG_ECHO,
      "%.32sPing: addr:%u.%u.%u.%u\n", now_str,
      ( address >> 24 ) & 0xFF, ( address >> 16 ) & 0xFF,
      ( address >> 8 ) & 0xFF, address & 0xFF );

    if ( address != __my_address )
    {
      CosmLog( __mylog, 1, COSM_LOG_ECHO,
        "%.32sNew address:%u.%u.%u.%u\n", now_str,
        ( address >> 24 ) & 0xFF, ( address >> 16 ) & 0xFF,
        ( address >> 8 ) & 0xFF, address & 0xFF );
      __my_address = address;
    }

    CosmSystemClock( &now );
    __last_checkin = now.hi;
  }

  CosmNetClose( net );
  __pinger_alive = 0;
}

EXPORT s32 CSSDKEngine( u32 machine_id, cosmtime expire, u32 * shutdown_flag,
  cosm_LOG * log, cosm_NET * net )
{
  cosmtime time_now;
  u64 thread_tracker, thread_pinger;

  __machine_id = machine_id;
  __expire = expire;
  __shutdown_flag = shutdown_flag;
  __mylog = log;

  CosmSystemClock( &time_now );
  __startup = time_now.hi;
  
  ReadLastShutdown();
  /* if we don't write the proper time, it's a crash */
  WriteLastShutdown( -1 );

  CosmLog( log, 0, COSM_LOG_ECHO, "%.32sRunning - Machine ID %08X.\n",
    Now(), __machine_id );

  CosmThreadBegin( &thread_pinger, Pinger, net, 64*1024 );
  /* let pinger get going before we start tracker */
  CosmSleep( 1000 );
  CosmThreadBegin( &thread_tracker, Tracker, NULL, 64*1024 );

  while ( ( *__shutdown_flag == 0 )
    || ( __tracker_alive == 1 )
    || ( __pinger_alive == 1 ) )
  {
    CosmSleep( 1000 );
  }

  /* now we can shutdown */
  CosmLog( log, 0, COSM_LOG_ECHO, "%.32sShutdown\n\n", Now() );
  CosmLogClose( log );

  /* successful shutdown */
  CosmSystemClock( &time_now );
  WriteLastShutdown( time_now.hi );
  
  if ( __expired_client )
  {
    return CSSDK_ENGINE_EXPIRED_CLIENT;
  }

  if ( __expired_engine )
  {
    return CSSDK_ENGINE_EXPIRED_ENGINE;
  }

  return CSSDK_ENGINE_SHUTDOWN;
}
