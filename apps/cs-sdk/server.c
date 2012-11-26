/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: CS-SDK - Client Server Software Development Kit

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "server.h"

/* thread states */
#define THREAD_STOPPED 0
#define THREAD_RUNNING 1
#define THREAD_END     -1
#define THREAD_DEAD    -2

/* how many threads to try and start */
#define THREAD_COUNT   16
#define THREAD_STACK   ( 32 * 1024 )


/* globals */
u32 __shutdown_flag;   /* interupt flag */
cosm_LOG __log_work;     /* for work results */
cosm_LOG __log_activity; /* for network activity, firewall triggers, etc */
cosm_NET_ACL __net_acl;  /* for evil hosts */
cosm_HTTPD __httpd;      /* httpd server */

/* thread data */
cosm_SEMAPHORE __thread_sem[THREAD_COUNT]; /* locks for threads */
cosm_NET __thread_net[THREAD_COUNT];       /* network handles for threads */
s32 __thread_state[THREAD_COUNT];          /* thread states */
u32 __thread_num[THREAD_COUNT];            /* tell the threads who they are */
u64 __thread_id[THREAD_COUNT];             /* thread id's */

/* mutexes for assign and accept of work */
cosm_MUTEX __mutex_assign;
cosm_MUTEX __mutex_accept;

/* work tracking data */
u32 __count;
u32 __count_end;
u64 __total;

void CatchSignal( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_INT, CatchSignal );
  CosmSignalRegister( COSM_SIGNAL_TERM, CatchSignal );

  /*
    do shutdown immediately (this may cause problems) but since we're in
    a ...Accept(...WAIT). We have no choice till we finish the
    multi-threaded example, or two-phase commit the work.
  */
  Shutdown();
  CosmProcessEnd( 0 );
}

void Thread( void * arg )
{
  u32 me;
  u32 type;
  u32 bytes;

  /* do thread setup */
  me = *( (u32 *) arg );
  CosmThreadPriority( 192 ); /* a little less then control thread */

  for ( ; ; )
  {
    /* wait for us to be told to run */
    CosmSemaphoreDown( &__thread_sem[me], COSM_SEMAPHORE_WAIT );

    /* check if we should exit now */
    if ( __thread_state[me] == THREAD_END )
    {
      CosmPrint( "Thread %u ending\n", me );
      CosmThreadEnd();
    }

    /* do the work we need to do */
    CosmPrint( "Hello from thread %u\n", me );

    while ( ( CosmNetRecv( &type, &bytes, &__thread_net[me],
      (u32) sizeof( type ), 5000 ) == COSM_PASS )
      && ( bytes == sizeof( type ) ) )
    {
      /* send off to subroutines */
      CosmU32Load( &type, (u8 *) &type );
      switch ( type )
      {
        case PACKET_TYPE_REQUEST:
          Assign( &__thread_net[me], NULL, 0 );
          break;
        case PACKET_TYPE_RESULTS:
          Accept( &__thread_net[me], NULL, 0 );
          break;
        default:
          /* error, close connection*/
          CosmNetClose( &__thread_net[me] );
          break;
      }
    }
    CosmNetClose( &__thread_net[me] );

    /* go back to sleep */
    __thread_state[me] = THREAD_STOPPED;
  }
}

s32 Handler( cosm_HTTPD_REQUEST * request )
{
  u32 type;
  u32 bytes;

  CosmHTTPDSendInit( request, 200, "OK", "text/html" );
  CosmHTTPDSendHead( request, "Cache-Control: no-cache" );

  /* read 4 bytes, call assign/accept */
  CosmPrint( "Hello from HTTP server\n" );

  if ( ( CosmHTTPDRecv( &type, &bytes, request, (u32) sizeof( type ), 5000 )
    == COSM_PASS ) && ( bytes == sizeof( type ) ) )
  {
    /* send off to subroutines */
    CosmU32Load( &type, (u8 *) &type );
    switch ( type )
    {
      case PACKET_TYPE_REQUEST:
        Assign( NULL, request, 1 );
        break;
      case PACKET_TYPE_RESULTS:
        Accept( NULL, request, 1 );
        break;
      default:
        /* error, close connection */
        return COSM_FAIL;
        break;
    }
  }

  CosmHTTPDSend( request, NULL, 0 );

  return COSM_PASS;
}

s32 Initialize( cosm_NET * net )
{
  cosm_NET_ADDR addr;

  /* load unassigned work data */

  /* load assigned/finished work data */

  /* setup network listener */
  if ( CosmNetDNS( &addr, 1, NETPACKET_SERVER ) != 1 )
  {
    CosmPrint( "Hostname lookup error.\n" );
    return COSM_FAIL;
  }

  if ( CosmU32Str( &addr.port, NULL, NETPACKET_PORT, 10 ) != COSM_PASS )
  {
    CosmPrint( "Port error.\n" );
    return COSM_FAIL;
  }

  if ( CosmNetListen( net, &addr, COSM_NET_MODE_TCP, 16 ) != COSM_PASS )
  {
    CosmPrint( "Network already in use.\n" );
    return COSM_FAIL;
  }

  CosmLog( &__log_activity, 0, COSM_LOG_ECHO,
    "Listening on address %u.%u.%u.%u port %u\n",
    ( net->my_addr.ip.v4 >> 24 ) & 0xFF, ( net->my_addr.ip.v4 >> 16 ) & 0xFF,
    ( net->my_addr.ip.v4 >> 8 ) & 0xFF, net->my_addr.ip.v4 & 0xFF,
    net->my_addr.port );

  return COSM_PASS;
}

s32 Shutdown( void )
{
  /* close down network */

  CosmHTTPDStop( &__httpd, 3000 );
  CosmHTTPDFree( &__httpd );

  /* save unassigned work data */

  /* save assigned/finished work data */

  return COSM_PASS;
}

void Assign( cosm_NET * net, cosm_HTTPD_REQUEST * http_req, u32 use_http )
{
  PACKET_REQUEST request;
  PACKET_ASSIGNMENT assign;
  u32 tmp_count;
  u32 length;
  u32 start;
  u32 bytes;

  /* read the rest of the request, assign the work from the unassigned work */
  length = (u32) ( sizeof( PACKET_REQUEST ) - sizeof( request.type ) );
  
  if ( use_http )
  {
    if ( ( CosmHTTPDRecv( &request.pad, &bytes, http_req, length,
      length + 5000 ) != COSM_PASS ) || ( bytes != length ) )
    {
      CosmPrint( "Network Recv Failed\n" );
      return;
    }
  }
  else
  {
    if ( ( CosmNetRecv( &request.pad, &bytes, net, length,
      length + 5000 ) != COSM_PASS ) || ( bytes != length ) )
    {
      CosmPrint( "Network Recv Failed\n" );
      return;
    }
  }

  CosmU64Load( &request.speed, (u8 *) &request.speed );
  assign.type = PACKET_TYPE_ASSIGNMENT;
  CosmU32Save( (u8 *) &assign.type, &assign.type );

  /* work with the global variables */
  CosmMutexLock( &__mutex_assign, COSM_MUTEX_WAIT );
  CosmU32Save( (u8 *) &assign.start, &__count );
  start = __count;
  /* already done? */
  if ( start >= __count_end )
  {
    tmp_count = 0;
  }
  else
  {
    tmp_count = __count + (u32) request.speed;
    if ( tmp_count >= __count_end )
    {
      __count = __count_end;
      tmp_count = __count_end - 1;
    }
    else
    {
      __count = tmp_count;
      tmp_count--;
    }
  }
  CosmMutexUnlock( &__mutex_assign );

  if ( tmp_count != 0 )
  {
    CosmPrint( "Assigned work %08X-%08X\n", start, tmp_count );
  }

  CosmU32Save( (u8 *) &assign.end, &tmp_count );

  if ( use_http )
  {
    if ( CosmHTTPDSend( http_req, &assign, (u32) sizeof( PACKET_ASSIGNMENT ) )
      != COSM_PASS )
    {
      CosmPrint( "Network Send Failed\n" );
      return;
    }
  }
  else
  {
    if ( CosmNetSend( net, &bytes, &assign, (u32) sizeof( PACKET_ASSIGNMENT ) )
      != COSM_PASS )
    {
      CosmPrint( "Network Send Failed\n" );
      return;
    }
  }
  /* mark what we've handed out as assigned work */

}

void Accept( cosm_NET * net, cosm_HTTPD_REQUEST * http_req, u32 use_http )
{
  PACKET_RESULTS results;
  PACKET_ACCEPT work_accept;
  const ascii * os_types[] = COSM_OS_TYPES;
  const ascii * cpu_types[] = COSM_CPU_TYPES;
  u32 length;
  u32 bytes;
  u64 tmp;

  /* accept and record results as finished */
  length = (u32) ( sizeof( PACKET_RESULTS ) - sizeof( results.type ) );

  if ( use_http )
  {
    if ( ( CosmHTTPDRecv( &results.start, &bytes, http_req, length,
      length + 5000 ) != COSM_PASS ) || ( bytes != length ) )
    {
      CosmPrint( "Network Recv Failed\n" );
      return;
    }
  }
  else
  {
    if ( ( CosmNetRecv( &results.start, &bytes, net, length,
      length + 5000 ) != COSM_PASS ) || ( bytes != length ) )
    {
      CosmPrint( "Network Recv Failed\n" );
      return;
    }
  }

  CosmU32Load( &results.start, (u8 *) &results.start );
  CosmU32Load( &results.end, (u8 *) &results.end );
  CosmU64Load( &results.total, (u8 *) &results.total );
  CosmU32Load( &results.cpu, (u8 *) &results.cpu );
  CosmU32Load( &results.os, (u8 *) &results.os );
  /* ... */
  CosmPrint(
    "Got one - Sum %016Y - OS = %.20s, CPU = %.20s [%.63s]\n",
    results.total, os_types[results.os], cpu_types[results.cpu],
    results.email );

  /* work with the global variables */
  CosmMutexLock( &__mutex_accept, COSM_MUTEX_WAIT );
  tmp = __total = ( __total + results.total );
  CosmMutexUnlock( &__mutex_accept );

  CosmPrint( "New total: %016Y\n", tmp );

  work_accept.type = PACKET_TYPE_ACCEPT;
  CosmU32Save( (u8 *) &work_accept.type, &work_accept.type );
  work_accept.result = COSM_PASS;
  CosmU32Save( (u8 *) &work_accept.result, &work_accept.result );

  if ( use_http )
  {
    if ( CosmHTTPDSend( http_req, &work_accept, (u32) sizeof( PACKET_ACCEPT ) )
      != COSM_PASS )
    {
      CosmPrint( "Network Send Failed\n" );
      return;
    }
  }
  else
  {
    if ( CosmNetSend( net, &bytes, &work_accept, (u32) sizeof( PACKET_ACCEPT ) )
      != COSM_PASS )
    {
      CosmPrint( "Network Send Failed\n" );
      return;
    }
  }

  /* log work done */

}

int main( int argc, char * argv[] )
{
  s32 result;
  cosm_NET net, new_net;
  u32 type;
  cosm_NET_HOSTNAME name;
  u32 i;
  u32 threads_started;
  u32 thread_seek, thread_tries, found;
  s32 err, err2;
  u32 bytes;
  cosm_NET_ADDR addr;
   
  /* run system tests */
  CosmPrint( "Running system tests... " );
  if ( CosmTest( &err, &err2, 0 ) != COSM_PASS )
  {
    CosmPrint( "Error: O=%u, C=%u, M=%i, E=%i\n",
      OS_TYPE, CPU_TYPE, err, err2 );
    CosmProcessEnd( -1 );
  }
  CosmPrint( "all passed.\n" );

  /* initialize the globals */
  CosmMemSet( &__log_work, sizeof( cosm_LOG ), 0 );
  CosmMemSet( &__log_activity, sizeof( cosm_LOG ), 0 );
  CosmMemSet( &__net_acl, sizeof( cosm_NET_ACL ) , 0 );
  __count = 0;
  __count_end = 0x10000000;
  __total = 0;
  CosmMemSet( &__mutex_assign, sizeof( cosm_MUTEX ), 0 );
  CosmMemSet( &__mutex_accept, sizeof( cosm_MUTEX ), 0 );
  for ( i = 0 ; i < THREAD_COUNT ; i++ )
  {
    CosmMemSet( &__thread_sem[i], sizeof( cosm_MUTEX ), 0 );
    CosmSemaphoreInit( &__thread_sem[i], 0 );
    __thread_id[i] = 0x0000000000000000LL;
    __thread_state[i] = THREAD_STOPPED;
    __thread_num[i] = i;
  }

  /* register signals, after this we have to check the flag for shutdown */
  __shutdown_flag = 0;
  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, CatchSignal ) != COSM_PASS ) ||
    ( CosmSignalRegister( COSM_SIGNAL_TERM, CatchSignal ) != COSM_PASS ) )
  {
    CosmPrint( "Signal registration failed\n" );
    CosmProcessEnd( -1 );
  }

  /* open logs */
  if ( CosmLogOpen( &__log_work, WORK_LOG, 100, COSM_LOG_MODE_NUMBER )
    != COSM_PASS )
  {
    CosmPrint( "Unable to open work log.\n" );
    CosmProcessEnd( -1 );
  }

  if ( CosmLogOpen( &__log_activity, ACTIVITY_LOG, 10, COSM_LOG_MODE_NUMBER )
    != COSM_PASS )
  {
    CosmPrint( "Unable to open activity log.\n" );
    CosmProcessEnd( -1 );
  }

  /* initialize network */
  CosmMemSet( &net, sizeof( cosm_NET ), 0 );
  CosmMemSet( &new_net, sizeof( cosm_NET ), 0 );
  if ( Initialize( &net ) != COSM_PASS )
  {
    CosmPrint( "Server Initialization failed\n" );
    CosmProcessEnd( -1 );
  }

  /* start the http listener */
  CosmNetDNS( &addr, 1, NETPACKET_HTTPSERVER );
  CosmU32Str( &addr.port, NULL, NETPACKET_HTTPPORT, 10 );
  CosmMemSet( &__httpd, sizeof( cosm_HTTPD ), 0 );
  CosmHTTPDInit( &__httpd, NULL, 0, THREAD_COUNT, THREAD_STACK, &addr, 5000 );
  CosmHTTPDSetHandler( &__httpd, "/", NULL, Handler );
  if ( CosmHTTPDStart( &__httpd, 3000 ) != COSM_PASS )
  {
    CosmPrint( "HTTPD Initialization failed\n" );
    CosmProcessEnd( -1 );
  }

  /* start the tcp threads */
  threads_started = 0;
  for ( i = 0 ; i < THREAD_COUNT ; i ++ )
  {
    /* note we cant just pass &i as a thread number */
    /* Make sure that the stack is large enough for your application! */
    if ( CosmThreadBegin( &__thread_id[i], Thread, &__thread_num[i],
      16 * 1024 ) != COSM_PASS )
    {
      CosmPrint( "%u was one thread too many\n", i + 1 );
      continue;
    }
    else
    {
      CosmPrint( "Thread %Y started\n", __thread_id[i] );
      threads_started++;
    }
  }

  if ( threads_started > 0 )
  {
    /* multi-thread version */
    thread_seek = 0;
    for ( ; ; )
    {
      found = 0;
      while ( found == 0 )
      {
        thread_tries = 0;
        thread_seek = ( thread_seek + 1 ) % threads_started;
        if ( __thread_state[thread_seek] == THREAD_STOPPED )
        {
          /* found an available thread */
          if ( CosmNetAccept( &__thread_net[thread_seek], &net, &__net_acl,
            COSM_NET_ACCEPT_WAIT ) == COSM_PASS )
          {
            CosmPrint( "Connection from %u.%u.%u.%u\n",
              ( net.addr.ip.v4 >> 24 ) & 0xFF,
              ( net.addr.ip.v4 >> 16 ) & 0xFF,
              ( net.addr.ip.v4 >> 8 ) & 0xFF,
              net.addr.ip.v4 & 0xFF );
            /* unleash thread */
            __thread_state[thread_seek] = THREAD_RUNNING;
            CosmSemaphoreUp( &__thread_sem[thread_seek] );
            thread_tries = 0;
            found = 1;
          }
        }
        else
        {
          /* try the next one, but do not busy-wait */
          if ( ++thread_tries > threads_started )
          {
            thread_tries = 0;
            CosmSleep( 10 );
          }
        }
      }
    }
  }
  else
  {
    /* no threads */
    while ( ( result = CosmNetAccept( &new_net, &net, &__net_acl,
      COSM_NET_ACCEPT_WAIT ) ) == COSM_PASS )
    {
      CosmNetRevDNS( &name, &new_net.addr );
      CosmPrint( "Connection from %u.%u.%u.%u\n",
        ( new_net.addr.ip.v4 >> 24 ) & 0xFF,
        ( new_net.addr.ip.v4 >> 16 ) & 0xFF,
        ( new_net.addr.ip.v4 >> 8 ) & 0xFF,
        new_net.addr.ip.v4 & 0xFF );

      while ( ( CosmNetRecv( &type, &bytes, &new_net, (u32) sizeof( type ),
        5000 ) == COSM_PASS ) && ( bytes == sizeof( type ) ) )
      {
        /* send off to subroutines */
        CosmU32Load( &type, (u8 *) &type );
        switch ( type )
        {
          case PACKET_TYPE_REQUEST:
            Assign( &new_net, NULL, 0 );
            break;
          case PACKET_TYPE_RESULTS:
            Accept( &new_net, NULL, 0 );
            break;
          default:
            /* error, close connection*/
            CosmNetClose( &new_net );
            break;
        }
      }
      CosmNetClose( &new_net );
    }
  }

  CosmProcessEnd( 0 );
  return COSM_PASS;
}
