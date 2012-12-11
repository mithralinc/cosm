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

#include "client.h"

/* globals */

#define POOL_SIZE 4

u32 __shutdown_flag;
u64 __speed;
cosm_CONFIG __config;
PACKET_ACTIVE __work_pool[POOL_SIZE];
u32 __use_http;

void catch_signal( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_INT, catch_signal );
  CosmSignalRegister( COSM_SIGNAL_TERM, catch_signal );
}

u32 GetWork( PACKET_ACTIVE * work, u32 count )
{
  u32 i, length, received, bytes;
  cosm_NET net;
  cosm_HTTP http;
  PACKET_REQUEST request;
  PACKET_ASSIGNMENT assign;
  u32 status;

  received = 0;

  /* open connection to server */
  CosmMemSet( &net, sizeof( cosm_NET ), 0 );
  CosmMemSet( &http, sizeof( cosm_HTTP ), 0 );
  if ( Connect( &net, &http ) != COSM_PASS )
  {
    return received;
  }

  /* save network data */
  request.type = PACKET_TYPE_REQUEST;
  CosmU32Save( (u8 *) &request.type, &request.type );
  CosmU64Save( (u8 *) &request.speed, &__speed );

  /* get up to count assignments */
  for ( i = 0 ; i < count ; i++ )
  {
    if ( work[i].state != STATE_INVALID )
    {
      /* dont overwrite good data, skip to next one */
      continue;
    }

    /* send request */
    if ( __use_http )
    {
      if ( CosmHTTPPost( &http, &status, "/", &request,
        (u32) sizeof( PACKET_REQUEST ), 5000 ) != COSM_PASS )
      {
        CosmPrint( "CosmHTTPPost failure\n" );
        CosmHTTPClose( &http );
        return received;
      }
    }
    else
    {
      if ( CosmNetSend( &net, &bytes, &request,
        (u32) sizeof( PACKET_REQUEST ) ) != COSM_PASS )
      {
        CosmPrint( "Network send failure\n" );
        CosmNetClose( &net );
        return received;
      }
    }

    /* read assignment */
    length = (u32) sizeof( PACKET_ASSIGNMENT );
    /* make sure to wait long enough, about 1sec/KB + 5sec */
    if ( __use_http )
    {
      if ( ( CosmHTTPRecv( &assign, &bytes, &http, length, length + 5000 )
        != COSM_PASS ) || ( bytes != length ) )
      {
        CosmPrint( "CosmHTTPRecv Timeout\n" );
        CosmHTTPClose( &http );
        return received;
      }
    }
    else
    {
      if ( ( CosmNetRecv( &assign, &bytes, &net, length, length + 5000 )
        != COSM_PASS ) || ( bytes != length ) )
      {
        CosmPrint( "Network Recv Timeout\n" );
        CosmNetClose( &net );
        return received;
      }
    }

    /* load network data */
    CosmU32Load( &assign.type, (u8 *) &assign.type );
    CosmU32Load( &assign.start, (u8 *) &assign.start );
    CosmU32Load( &assign.end, (u8 *) &assign.end );

    if ( assign.type != PACKET_TYPE_ASSIGNMENT )
    {
      if ( __use_http )
      {
        CosmHTTPClose( &http );
      }
      else
      {
        CosmNetClose( &net );
      }
      CosmPrint( "Bad block type\n" );
      return received;
    }

    if ( assign.end == 0 )
    {
      if ( __use_http )
      {
        CosmHTTPClose( &http );
      }
      else
      {
        CosmNetClose( &net );
      }
      CosmPrint( "Server out of work\n" );
      return received;
    }

    /* copy into a PACKET_ACTIVE - the Runable data type */
    work[i].type = PACKET_TYPE_ACTIVE;
    work[i].start = assign.start;
    work[i].current = assign.start;
    work[i].end = assign.end;
    work[i].total = (u64) 0;
    work[i].state = STATE_ALIVE;

    CosmPrint( "Receieved assignment block %u/%u\n",
      received + 1, count );
    CosmPrint( "current %08X / end %08X\n",
      work[i].current, work[i].end );

    received++;
  }

  if ( __use_http )
  {
    CosmHTTPClose( &http );
  }
  else
  {
    CosmNetClose( &net );
  }
  return received;
}

u32 PutWork( PACKET_ACTIVE * work, u32 count )
{
  u32 i, length, sent, bytes;
  cosm_NET net;
  cosm_HTTP http;
  PACKET_RESULTS results;
  PACKET_ACCEPT work_accept;
  u32 status;

  sent = 0;

  /* open connection to server */
  CosmMemSet( &net, sizeof( cosm_NET ), 0 );
  CosmMemSet( &http, sizeof( cosm_HTTP ), 0 );
  if ( Connect( &net, &http ) != COSM_PASS )
  {
    return sent;
  }

  /* get up to count assignments */
  for ( i = 0 ; i < count ; i++ )
  {
    if ( work[i].state != STATE_DONE )
    {
      /* it's not done, done send this one back, move on to next one */
      continue;
    }

    /* save network data */
    results.type = PACKET_TYPE_RESULTS;
    CosmU32Save( (u8 *) &results.type, &results.type );
    CosmU32Save( (u8 *) &results.start, &work[i].start );
    CosmU32Save( (u8 *) &results.end, &work[i].end );
    CosmU64Save( (u8 *) &results.total, &work[i].total );
    results.cpu = CPU_TYPE;
    CosmU32Save( (u8 *) &results.cpu, &results.cpu );
    results.os = OS_TYPE;
    CosmU32Save( (u8 *) &results.os, &results.os );
    CosmStrCopy( results.email, CosmConfigGet( &__config, "settings",
      "email" ), sizeof( results.email ) );

    /* send request */
    if ( __use_http )
    {
      if ( CosmHTTPPost( &http, &status, "/", &results,
        (u32) sizeof( PACKET_RESULTS ), 5000 ) != COSM_PASS )
      {
        CosmPrint( "CosmHTTPPost failure\n" );
        CosmHTTPClose( &http );
        return sent;
      }
    }
    else
    {
      if ( CosmNetSend( &net, &bytes, &results,
        (u32) sizeof( PACKET_RESULTS ) ) != COSM_PASS )
      {
        CosmPrint( "Network send failure\n" );
        CosmNetClose( &net );
        return sent;
      }
    }

    /* read assignment */
    length = (u32) sizeof( PACKET_ACCEPT );
    /* make sure to wait long enough, about 1sec/KB + 5sec */
    if ( __use_http )
    {
      if ( ( CosmHTTPRecv( &work_accept, &bytes, &http, length, length + 5000 )
        != COSM_PASS ) || ( bytes != length ) )
      {
        CosmPrint( "CosmHTTPRecv Timeout\n" );
        CosmHTTPClose( &http );
        return sent;
      }
    }
    else
    {
      if ( ( CosmNetRecv( &work_accept, &bytes, &net, length, length + 5000 )
        != COSM_PASS ) || ( bytes != length ) )
      {
        CosmPrint( "Network Recv Timeout\n" );
        CosmNetClose( &net );
        return sent;
      }
    }

    /* load network data */
    CosmU32Load( &work_accept.type, (u8 *) &work_accept.type );
    CosmU32Load( &work_accept.result, (u8 *) &work_accept.result );

    if ( work_accept.type != PACKET_TYPE_ACCEPT )
    {
      CosmPrint( "Bad block type\n" );
      if ( __use_http )
      {
        CosmHTTPClose( &http );
      }
      else
      {
        CosmNetClose( &net );
      }
      return sent;
    }

    /* if work_accept.result == COSM_PASS */
    CosmPrint( "Sent result block %u/%u\n", sent + 1, count );
    CosmPrint( "start %08X / end %08X\n",
      work[i].start, work[i].end );

    work[i].state = STATE_INVALID;

    sent++;
  }

  if ( __use_http )
  {
    CosmHTTPClose( &http );
  }
  else
  {
    CosmNetClose( &net );
  }
  return sent;
}

s32 Connect( cosm_NET * net, cosm_HTTP * http )
{
  cosm_NET_ADDR addr, fire_addr;
  s32 result;
  ascii url[512];
  const ascii * tmp_str;

  CosmMemSet( &addr, sizeof( addr ), 0 );
  CosmMemSet( &fire_addr, sizeof( fire_addr ), 0 );

  /* check if we're using http tunneling */
  if ( CosmStrStr( CosmConfigGet( &__config, "firewall",
    "mode" ), "http", 10LL ) != NULL )
  {
    __use_http = 1;
    CosmPrintStr( url, (u64) 512, "http://%.512s:%.8s/",
      CosmConfigGet( &__config, "settings", "httpserver" ),
      CosmConfigGet( &__config, "settings", "httpport" ) );
    addr.port = 80;

    if ( CosmStrStr( CosmConfigGet( &__config, "firewall",
      "mode" ), "proxy", 10LL ) != NULL )
    {
      if ( ( ( tmp_str = CosmConfigGet( &__config, "http",
        "host" ) ) != NULL ) && ( CosmStrBytes( tmp_str ) > 0 ) )
      {
        if ( CosmNetDNS( &fire_addr, 1, (ascii *) tmp_str ) != 1 )
        {
          CosmPrint( "Firewall (HTTP) hostname lookup error.\n" );
          return COSM_FAIL;
        }
      }
      if ( ( ( tmp_str = CosmConfigGet( &__config, "http",
        "port" ) ) != NULL ) && ( CosmStrBytes( tmp_str ) > 0 ))
      {
        if ( CosmU32Str( &fire_addr.port, NULL, (ascii *) tmp_str, 10 )
          != COSM_PASS )
        {
          CosmPrint( "Firewall (HTTP) port error.\n" );
          return COSM_FAIL;
        }
      }
    }
    if ( ( result = CosmHTTPOpen( http, url, &fire_addr,
      NULL, NULL, NULL, NULL ) ) != COSM_PASS )
    {
      switch ( result )
      {
        case COSM_HTTP_ERROR_URI:
        case COSM_HTTP_ERROR_PARAM:
          CosmPrint( "Network setting error.\n" );
          break;
        case COSM_HTTP_ERROR_NET:
        case COSM_HTTP_ERROR_MEMORY:
        case COSM_HTTP_ERROR_CLOSED:
          CosmPrint( "System unable to open network connection.\n" );
          break;
        default:
          CosmPrint( "Error opening network connection.\n" );
          break;
      }
      return COSM_FAIL;
    }
  }
  else
  {
    __use_http = 0;
    if ( CosmNetDNS( &addr, 1, (ascii *) CosmConfigGet( &__config,
      "settings", "server" ) ) != 1 )
    {
      CosmPrint( "Hostname lookup error.\n" );
      return COSM_FAIL;
    }

    if ( CosmU32Str( &addr.port, NULL, CosmConfigGet( &__config, "settings",
      "port" ), 10 ) != COSM_PASS )
    {
      CosmPrint( "Port error.\n" );
      return COSM_FAIL;
    }

    /* check is firewall is active */
    if ( CosmStrCmp( "socks", CosmConfigGet( &__config,
      "firewall", "mode" ), (u64) 5 ) == 0 )
    {
      if ( CosmNetDNS( &fire_addr, 1, (ascii *) CosmConfigGet( &__config,
        "socks", "host" ) ) != 1 )
      {
        CosmPrint( "Firewall (SOCKS) hostname lookup error.\n" );
        return COSM_FAIL;
      }

      if ( CosmU32Str( &fire_addr.port, NULL, CosmConfigGet( &__config,
        "socks", "port" ), 10 ) != COSM_PASS )
      {
        CosmPrint( "Firewall (SOCKS) port error.\n" );
        return COSM_FAIL;
      }

     CosmPrint( "Connect: %u.%u.%u.%u/%u - fire %u.%u.%u.%u/%u\n",
      ( addr.ip.v4 >> 24 ) & 0xFF, ( addr.ip.v4 >> 16 ) & 0xFF,
      ( addr.ip.v4 >> 8 ) & 0xFF, addr.ip.v4 & 0xFF, addr.port,
      ( fire_addr.ip.v4 >> 24 ) & 0xFF, ( fire_addr.ip.v4 >> 16 ) & 0xFF,
      ( fire_addr.ip.v4 >> 8 ) & 0xFF, fire_addr.ip.v4 & 0xFF,
      fire_addr.port );

      result = CosmNetOpenSocks( net, &addr, &fire_addr,
        (ascii *) CosmConfigGet( &__config, "socks", "userpass" ) );
    }
    else
    {
     CosmPrint( "Connect: %u.%u.%u.%u port %u\n",
      ( addr.ip.v4 >> 24 ) & 0xFF, ( addr.ip.v4 >> 16 ) & 0xFF,
      ( addr.ip.v4 >> 8 ) & 0xFF, addr.ip.v4 & 0xFF, addr.port );

      result = CosmNetOpen( net, NULL, &addr, COSM_NET_MODE_TCP );
    }

    if ( result != COSM_PASS )
    {
      switch ( result )
      {
        case COSM_NET_ERROR_ADDRESS:
        case COSM_NET_ERROR_MYADDRESS:
          CosmPrint( "Network setting error.\n" );
          break;
        case COSM_NET_ERROR_FIREHOST:
        case COSM_NET_ERROR_FIREPORT:
        case COSM_NET_ERROR_FIREPASS:
          CosmPrint( "Firewall setting error.\n" );
          break;
        case COSM_NET_ERROR_FATAL:
        case COSM_NET_ERROR_SOCKET:
        case COSM_NET_ERROR_NO_NET:
          CosmPrint( "System unable to open network connection.\n" );
          break;
        default:
          CosmPrint( "Error opening network connection.\n" );
          break;
      }
      return COSM_FAIL;
    }
  }
  return COSM_PASS;
}

u32 Load( void )
{
  /* Load in all PACKET_ACTIVE's from CLIENT_DATAFILE */
  /* Load all data into correct format using Cosm{type}Load() */

  return 0;
}

s32 Save( void )
{
  /* Save all PACKET_ACTIVE's to CLIENT_DATAFILE */
  /* Save all data into correct format using Cosm{type}Save() */

  return COSM_PASS;
}

s32 Run( PACKET_ACTIVE * work, u64 iterations )
{
  u64 loops;

  if ( work->type != PACKET_TYPE_ACTIVE )
  {
    return STATE_INVALID;
  }

  if ( work->state != STATE_ALIVE )
  {
    return STATE_DONE;
  }

  loops = 0x0000000000000000LL;

  /* while work to do, not interupted, and not over our iteration limit */
  while ( ( work->current <= work->end ) && ( __shutdown_flag == 0 ) &&
    ( ( loops < iterations ) ) )
  {
    work->total = work->total + work->current;
    work->current++;

    loops++;
  }

  /* if out of work to do */
  if ( work->current > work->end )
  {
    work->state = STATE_DONE;
  }

  return STATE_ALIVE;
}

u64 Speed( void )
{
  PACKET_ACTIVE test;
  cosmtime start, end, diff, span;
  u128 bigcount, udiff;
  u64 count, limit;

  CosmPrint( "Benchmarking " );

  /* default speed */
  count = (u64) 0x10000;

  /*
    benckmark over a 1 second span, this will limit our resolution.
    test will take max of 2 x span.
    If the work being done takes a large time relative to a second, then
    you will need to benchmark over a longer period to get the needed
    resolution.
  */
  _COSM_SET128( span, 0000000000000001, 0000000000000000 );

  limit = 0xFFFFFFFFFFFFFFFFLL;
  do
  {
    CosmPrint( "." );
    count = ( count << 1 );
    test.type = PACKET_TYPE_ACTIVE;
    test.current = 0;
    test.end = (u32) count;
    test.total = (u64) 0;
    test.state = STATE_ALIVE;
/* !!! need to deal with error cases of clock or Run() */
    CosmSystemClock( &start );
    Run( &test, limit );
    CosmSystemClock( &end );
    diff = CosmS128Sub( end, start );
  } while ( CosmS128Lt( diff, span ) && ( __shutdown_flag == 0 ) );

  /* make an unsigned udiff from diff */
  udiff = CosmU128S128( diff );

  /*
    calculate how many per unit time, this determines how much work
    we request, change the 1 below to a 86400 or 1 day of work at a time.
    It's safe to assume that if we can do 2^64 units in a day, then we sure
    do not need client-server to do it :)
  */
  bigcount.hi = count;
  bigcount.lo = 0;
  bigcount = CosmU128Div( bigcount, udiff );
  count = bigcount.lo;

  /* make sure it's more then 0 */
  if ( count == 0 )
  {
    count++;
  }

  CosmPrint( "\nSpeed = %v\n", count );

  return count;
}

s32 Test( void )
{
  PACKET_ACTIVE test;
  u64 correct, limit;
  s32 err;

  /* Run with some test data, be sure it's more then one Run() worth */
  test.type = PACKET_TYPE_ACTIVE;
  test.current = 0;
  test.end = 0xFFFF;
  test.total = (u64) 0;
  test.state = STATE_ALIVE;
  limit = 0x0000000000001000LL;
  do
  {
    err = Run( &test, limit );
  } while ( ( err == STATE_ALIVE ) && ( __shutdown_flag == 0 ) );

  correct = 0x000000007FFF8000LL;
  if ( ( test.total != correct ) )
  {
    return -1;
  }

  /* need more tests */

  return COSM_PASS;
}

s32 Config( void )
{
  ascii buffer[64];
  u32 length = 64;

  CosmPrint( "Email/ID [%.63s]: ", CosmConfigGet( &__config,
    "settings", "email" ) );
  CosmInput( buffer, length, COSM_IO_ECHO );
  if ( CosmStrBytes( buffer ) > 0 )
  {
    CosmConfigSet( &__config, "settings", "email", buffer );
    CosmConfigSave( &__config, CLIENT_CONFIG );
  }

  CosmPrint( "Server [%.63s]: ", CosmConfigGet( &__config,
    "settings", "server" ) );
  CosmInput( buffer, length, COSM_IO_ECHO );
  if ( CosmStrBytes( buffer ) > 0 )
  {
    CosmConfigSet( &__config, "settings", "server",
      buffer );
    CosmConfigSave( &__config, CLIENT_CONFIG );
  }

  CosmPrint( "Port [%.5s]: ", CosmConfigGet( &__config,
    "settings", "port" ) );
  CosmInput( buffer, length, COSM_IO_ECHO );
  if ( CosmStrBytes( buffer ) > 0 )
  {
    CosmConfigSet( &__config, "settings", "port", buffer );
    CosmConfigSave( &__config, CLIENT_CONFIG );
  }

  CosmPrint( "Logging level [%.5s]: ", CosmConfigGet( &__config,
    "settings", "loglevel" ) );
  CosmInput( buffer, length, COSM_IO_ECHO );
  if ( CosmStrBytes( buffer ) > 0 )
  {
    CosmConfigSet( &__config, "settings", "loglevel",
      buffer );
    CosmConfigSave( &__config, CLIENT_CONFIG );
  }

  CosmPrint(
    "Communication mode (normal/socks/http/proxy-http) [%.63s]: ",
    CosmConfigGet( &__config, "firewall", "mode" ) );
  CosmInput( buffer, length, COSM_IO_ECHO );
  if ( CosmStrBytes( buffer ) > 0 )
  {
    /* clean input */
    if ( ( buffer[0] == 'p' ) || ( buffer[0] == 'P' ) )
    {
      CosmConfigSet( &__config, "firewall", "mode",
        "proxyhttp" );
    }
    else if ( ( buffer[0] == 'h' ) || ( buffer[0] == 'H' ) )
    {
      CosmConfigSet( &__config, "firewall", "mode",
        "http" );
    }
    else if ( ( buffer[0] == 's' ) || ( buffer[0] == 'S' ) )
    {
      CosmConfigSet( &__config, "firewall", "mode",
        "socks" );
    }
    else
    {
      CosmConfigSet( &__config, "firewall", "mode",
        "normal" );
    }
    CosmConfigSave( &__config, CLIENT_CONFIG );
  }

  if ( CosmStrCmp( CosmConfigGet( &__config, "firewall",
    "mode" ), "socks", (u64) 5 ) == 0 )
  {
    CosmPrint( "SOCKS host [%.63s]: ", CosmConfigGet( &__config,
      "socks", "host" ) );
    CosmInput( buffer, length, COSM_IO_ECHO );
    if ( CosmStrBytes( buffer ) > 0 )
    {
      CosmConfigSet( &__config, "socks", "host",
        buffer );
      CosmConfigSave( &__config, CLIENT_CONFIG );
    }

    CosmPrint( "SOCKS port [%.63s]: ", CosmConfigGet( &__config,
      "socks", "port" ) );
    CosmInput( buffer, length, COSM_IO_ECHO );
    if ( CosmStrBytes( buffer ) > 0 )
    {
      CosmConfigSet( &__config, "socks", "port",
        buffer );
      CosmConfigSave( &__config, CLIENT_CONFIG );
    }

    CosmPrint( "SOCKS user:pass [%.63s]: ", CosmConfigGet(
      &__config, "socks", "userpass" ) );
    CosmInput( buffer, length, COSM_IO_ECHO );
    if ( CosmStrBytes( buffer ) > 0 )
    {
      CosmConfigSet( &__config, "socks", "userpass",
        buffer );
      CosmConfigSave( &__config, CLIENT_CONFIG );
    }
  }

  if ( CosmStrStr( CosmConfigGet( &__config, "firewall",
    "mode" ), "http", 10LL ) != NULL )
  {
    CosmPrint( "HTTP server [%.63s]: ", CosmConfigGet( &__config,
      "settings", "httpserver" ) );
    CosmInput( buffer, length, COSM_IO_ECHO );
    if ( CosmStrBytes( buffer ) > 0 )
    {
      CosmConfigSet( &__config, "settings", "httpserver",
        buffer );
      CosmConfigSave( &__config, CLIENT_CONFIG );
    }

    CosmPrint( "HTTP port [%.5s]: ", CosmConfigGet( &__config,
      "settings", "httpport" ) );
    CosmInput( buffer, length, COSM_IO_ECHO );
    if ( CosmStrBytes( buffer ) > 0 )
    {
      CosmConfigSet( &__config, "settings", "httpport", buffer );
      CosmConfigSave( &__config, CLIENT_CONFIG );
    }

    if ( CosmStrStr( CosmConfigGet( &__config, "firewall",
      "mode" ), "proxy", 10LL ) != NULL )
    {
      CosmPrint( "HTTP proxy host [%.63s]: ", CosmConfigGet( &__config,
        "http", "host" ) );
      CosmInput( buffer, length, COSM_IO_ECHO );
      if ( CosmStrBytes( buffer ) > 0 )
      {
        CosmConfigSet( &__config, "http", "host",
          buffer );
        CosmConfigSave( &__config, CLIENT_CONFIG );
      }

      CosmPrint( "HTTP proxy port [%.63s]: ", CosmConfigGet( &__config,
        "http", "port" ) );
      CosmInput( buffer, length, COSM_IO_ECHO );
      if ( CosmStrBytes( buffer ) > 0 )
      {
        CosmConfigSet( &__config, "http", "port",
          buffer );
        CosmConfigSave( &__config, CLIENT_CONFIG );
      }
    }
  }

  return COSM_PASS;
}

int main( int argc, char * argv[] )
{
  u32 i;
  u32 count;
  s32 err, err2;
  u64 limit;

  /* run system tests */
  CosmPrint( "Running system tests... " );
  if ( CosmTest( &err, &err2, 0 ) != COSM_PASS )
  {
    CosmPrint( "\nPlease report error: O=%u, C=%u, M=%i, E=%i\n",
      OS_TYPE, CPU_TYPE, err, err2 );
    CosmProcessEnd( -1 );
  }
  CosmPrint( "all passed.\n" );

  /* initialize the globals */
  CosmMemSet( &__config, sizeof( cosm_CONFIG ), 0 );
  CosmMemSet( &__work_pool, sizeof( PACKET_ACTIVE ) * POOL_SIZE, 0 );

  /* set client to lowest priority */
  CosmProcessPriority( 1 );

  /* register signals, after this we have to check the flag for shutdown */
  __shutdown_flag = 0;
  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, catch_signal ) != COSM_PASS )
    || ( CosmSignalRegister( COSM_SIGNAL_TERM, catch_signal ) != COSM_PASS ) )
  {
    CosmPrint( "Signal registration failed\n" );
    CosmProcessEnd( -1 );
  }

  CosmPrint( "Running self-tests... " );
  if ( ( err = Test() ) != COSM_PASS )
  {
    CosmPrint( "Please report error: O=%u, C=%u, P=%i\n",
      OS_TYPE, CPU_TYPE, err );
    CosmProcessEnd( -1 );
  }
  CosmPrint( "all passed.\n", err );

  /* load and check __config */
  CosmConfigLoad( &__config, CLIENT_CONFIG );
  if ( CosmConfigGet( &__config, "settings", "email" )
    == NULL )
  {
    /* set defaults */
    CosmConfigSet( &__config, "settings", "email",
      "cs-sdk" );
    CosmConfigSet( &__config, "settings", "server",
      NETPACKET_SERVER );
    CosmConfigSet( &__config, "settings", "port",
      NETPACKET_PORT );
    CosmConfigSet( &__config, "settings", "httpserver",
      NETPACKET_HTTPSERVER );
    CosmConfigSet( &__config, "settings", "httpport",
      NETPACKET_HTTPPORT );
    CosmConfigSet( &__config, "settings", "loglevel",
      "0" );
    CosmConfigSet( &__config, "firewall", "mode",
      "normal" );
    CosmConfigSet( &__config, "socks", "host",
      "hostname" );
    CosmConfigSet( &__config, "socks", "port",
      "1080" );
    CosmConfigSet( &__config, "socks", "userpass",
      "user:password" );
    CosmConfigSet( &__config, "http", "host",
      "" );
    CosmConfigSet( &__config, "http", "port",
      "" );
    CosmConfigSave( &__config, CLIENT_CONFIG );
    Config();
  }

  if ( ( argc == 2 ) && ( CosmStrCmp( "-config", (ascii *) argv[1],
    16LL ) == 0 ) )
  {
    Config();
  }

  /* how much work do we want, do it here to minimize connection time later */
  /* calculate 24 hours of work for __speed */
  /*
  __speed = CosmU64Mul( __speed, 86400LL );
  */
  /* set limit to about 15 minutes of work */
  /*
  limit = CosmU64Mul( __speed, 900LL );
  */

  /* use fast numbers for example, 1 sec of work and 1/4 second limit */
  __speed = Speed();
  limit = __speed / 4;

  while ( __shutdown_flag == 0 )
  {
    if ( ( count = GetWork( &__work_pool[0], POOL_SIZE ) ) == 0 )
    {
      CosmPrint( "GetWork Failed\n" );
      CosmProcessEnd( -1 );
    }

    for ( i = 0 ; i < count ; i++ )
    {
      while ( ( Run( &__work_pool[i], limit ) == STATE_ALIVE ) &&
        ( __shutdown_flag == 0 ) )
      {
        /* checkpoint */
        Save();
        CosmPrint( "%u Sum %016Y\n", i, __work_pool[i].total );
      }
    }

    PutWork( &__work_pool[0], POOL_SIZE );
  }

  CosmProcessEnd( 0 );
  return COSM_PASS;
}
