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

#include "cosm/os_net.h"
#include "cosm/os_mem.h"
#include "cosm/os_io.h"

#if ( !defined( NO_NETWORKING ) )
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <iphlpapi.h>
#define close closesocket
#else
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#if ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_MACOSX ) )
#include <net/if.h>
#include <ifaddrs.h>
#endif
#endif /* OS */
#endif /* NO_NETWORKING */

/* global networking initialization and mutex if no IPv6 */
u32 __cosm_net_global_init = 0;

s32 CosmNetOpen( cosm_NET * net, cosm_NET_ADDR * my_addr,
  const cosm_NET_ADDR * addr, u32 mode )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor, option;
  unsigned int local_addr_length, remote_addr_length;
  struct sockaddr_in local_addr4, remote_addr4;
  struct sockaddr_in6 local_addr6, remote_addr6;

  if ( ( NULL == net ) || ( NULL == addr )
    || ( ( COSM_NET_MODE_TCP != mode ) && ( COSM_NET_MODE_UDP != mode ) ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( COSM_NET_STATUS_CLOSED != net->status )
  {
    return COSM_NET_ERROR_ORDER;
  }

  if ( COSM_PASS != Cosm_NetGlobalInit() )
  {
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_NO_NET;
  }

  if ( COSM_NET_IPV4 == addr->type )
  {
    if ( COSM_NET_MODE_TCP == mode )
    {
      socket_descriptor = (int) socket( PF_INET, SOCK_STREAM, 0 );
    }
    else /* UDP */
    {
      socket_descriptor = (int) socket( PF_INET, SOCK_DGRAM, 0 );
    }
  }
  else if ( COSM_NET_IPV6 == addr->type )
  {
    if ( COSM_NET_MODE_TCP == mode )
    {
      socket_descriptor = (int) socket( PF_INET6, SOCK_STREAM, 0 );
    }
    else /* UDP */
    {
      socket_descriptor = (int) socket( PF_INET6, SOCK_DGRAM, 0 );
    }
  }
  else
  {
    return COSM_NET_ERROR_ADDRTYPE;
  }
  
  if ( -1 == socket_descriptor )
  {
    /* unable to open the socket */
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_SOCKET;
  }
  else
  {
    net->status = COSM_NET_STATUS_OPENING;
  }

  net->handle = (u64) socket_descriptor;

  if ( NULL != my_addr )
  {
    if ( COSM_NET_IPV4 == my_addr->type  )
    {
      /* Bind to a specified local host/port */
      CosmMemSet( &local_addr4, sizeof( local_addr4 ), 0 );
      local_addr_length = sizeof( local_addr4 );
      local_addr4.sin_family = PF_INET;
      local_addr4.sin_addr.s_addr = htonl( my_addr->ip.v4 );
      local_addr4.sin_port = htons( (u16) my_addr->port );

      /* allow us to bind more than one socket to the same local port */
      option = 1;
      if ( -1 == setsockopt( socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &option, sizeof( option ) ) )
      {
        /* unable to set the socket REUSEADDR option */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }

      if ( -1 == bind( socket_descriptor, (struct sockaddr *) &local_addr4,
        local_addr_length ) )
      {
        /* unable to bind to the socket */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_MYADDRESS;
      }
    }
    else if ( COSM_NET_IPV6 == addr->type )
    {
      /* Bind to a specified local host/port */
      CosmMemSet( &local_addr6, sizeof( local_addr6 ), 0 );
      local_addr_length = sizeof( local_addr6 );
      local_addr6.sin6_family = AF_INET6;
      CosmU128Save( &local_addr6.sin6_addr.s6_addr, &my_addr->ip.v6 );
      CosmU16Save( &local_addr6.sin6_port, &my_addr->port );

      /* allow us to bind more than one socket to the same local port */
      option = 1;
      if ( -1 == setsockopt( socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &option, sizeof( option ) ) )
      {
        /* unable to set the socket REUSEADDR option */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }

      if ( -1 == bind( socket_descriptor, (struct sockaddr *) &local_addr6,
        local_addr_length ) )
      {
        /* unable to bind to the socket */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_MYADDRESS;
      }    
    }
    else
    {
      Cosm_NetClose( net );
      return COSM_NET_ERROR_MYADDRESS;
    }
  }

  if ( COSM_NET_IPV4 == addr->type )
  {
    if ( COSM_NET_MODE_TCP == mode )
    {
      /* Connect to remote host */
      CosmMemSet( &remote_addr4, sizeof( remote_addr4 ), 0 );
      remote_addr_length = sizeof( remote_addr4 );
      remote_addr4.sin_family = AF_INET;
      remote_addr4.sin_addr.s_addr = htonl( addr->ip.v4 );
      remote_addr4.sin_port = htons( (u16) addr->port );

      if ( -1 == connect( socket_descriptor, (struct sockaddr *) &remote_addr4,
        remote_addr_length ) )
      {
        /* Error connecting to remote host */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_ADDRESS;
      }
    }

    if ( NULL != my_addr )
    {
      /* get which local address was actually used */
      CosmMemSet( &local_addr4, sizeof( local_addr4 ), 0 );
      local_addr_length = sizeof( local_addr4 );

      if ( -1 == getsockname( socket_descriptor, (struct sockaddr *)
        &local_addr4, &local_addr_length ) )
      {
        /* unable to get the local address and port of the socket */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      net->my_addr.type = COSM_NET_IPV4;
      net->my_addr.ip.v4 = ntohl( local_addr4.sin_addr.s_addr );
      net->my_addr.port = ntohs( local_addr4.sin_port );
    }
  }
  else /* IPv6 */
  {
    if ( COSM_NET_MODE_TCP == mode )
    {
      /* Connect to remote host */
      CosmMemSet( &remote_addr6, sizeof( remote_addr6 ), 0 );
      remote_addr_length = sizeof( remote_addr6 );
      remote_addr6.sin6_family = AF_INET6;
      CosmU128Save( &local_addr6.sin6_addr.s6_addr, &my_addr->ip.v6 );
      CosmU16Save( &local_addr6.sin6_port, &my_addr->port );

      if ( -1 == connect( socket_descriptor, (struct sockaddr *) &remote_addr6,
        remote_addr_length ) )
      {
        /* Error connecting to remote host */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_ADDRESS;
      }
    }

    if ( NULL != my_addr )
    {
      /* get which local address was actually used */
      CosmMemSet( &local_addr6, sizeof( local_addr6 ), 0 );
      local_addr_length = sizeof( local_addr6 );

      if ( -1 == getsockname( socket_descriptor, (struct sockaddr *)
        &local_addr6, &local_addr_length ) )
      {
        /* unable to get the local address and port of the socket */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      net->my_addr.type = COSM_NET_IPV6;
      CosmU128Load( &my_addr->ip.v6, &remote_addr6.sin6_addr.s6_addr );
      CosmU16Load( &my_addr->port, &remote_addr6.sin6_port );
    }
  }

  net->addr = *addr;
  net->mode = mode;
  net->status = COSM_NET_STATUS_OPEN;
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetOpenSocks( cosm_NET * net, const cosm_NET_ADDR * host,
  const cosm_NET_ADDR * firehost, const ascii * firepass )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  ascii receive_buffer[32], send_buffer[74];
  ascii username[COSM_NET_PASS_LENGTH], password[COSM_NET_PASS_LENGTH];
  u8 send_buffer_length;
  u32 wait_time;
  u32 i, j;
  u16 * temp_u16_ptr;
  u32 * temp_u32_ptr;
  s32 error;
  u32 bytes;

  if ( ( NULL == net ) || ( NULL == host ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( net->status != COSM_NET_STATUS_CLOSED )
  {
    return COSM_NET_ERROR_ORDER;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_NO_NET;
  }

  if ( ( NULL == firehost ) || ( NULL == firepass ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  /*
    socks4 send_buf: ver + command + port + ip + password (NULL terminated)
     = 1 + 1 + 2 + 4 + [1-64]
  */

  /*
    Using a firewall to connect to host. Temporary way to tell recv
    how long to wait for an answer from the firewall
  */
  wait_time = 120000; /* 120 seconds */

  if ( firehost->port == 0 )
  {
    /* Invalid port for the firewall */
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_FIREPORT;
  }
  if ( ( CosmStrBytes( firepass ) + 1 ) > COSM_NET_PASS_LENGTH )
  {
    /* Firewall password is too long */
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_FIREPASS;
  }

  /* Separating username from password for socks5 firewall */
  for ( i = 0 ; i < CosmStrBytes( firepass ) ; i++ )
  {
    if ( firepass[i] != ';' )
    {
      username[i] = firepass[i];
    }
    else
    {
      username[i] = 0;
      i++;
      break;
    }
  }
  username[i] = 0;

  for ( j = 0 ; i < CosmStrBytes( firepass ) ; i++ )
  {
    if ( firepass[i] == ';' )
    {
      /* Can't have two semi-colon */
      net->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_FIREPASS;
    }
    password[j] = firepass[i];
    j++;
  }
  password[j] = 0;
  if ( j == 0 )
  {
    /* No semi-colon means socks4 server connection */
    password[j + 1] = 0;
  }
  else
  {
    /* Just in case it's using socks5 with no password */
    password[j + 1] = 1;
  }

  /* Connecting to firewall */
  if ( COSM_PASS != ( error = CosmNetOpen( net, NULL, firehost,
    COSM_NET_MODE_TCP ) ) )
  {
    /* Unable to connect to firewall */
    if ( error == COSM_NET_ERROR_ADDRESS )
    {
      error = COSM_NET_ERROR_FIREHOST;
    }
    return error;
  }

  /* Using socks4 or socks5? */
  if ( ( password[0] != 0 ) || ( password[1] != 0 ) )
  {
    /* Socks5 */
    /* Sending a user/password authentication request for socks5 servers */
    send_buffer[0] = 0x05;
    send_buffer[1] = 0x01;
    send_buffer[2] = 0x02;
    if ( CosmNetSend( net, &bytes, send_buffer, 3 ) != COSM_PASS )
    {
      /* Unable to send through the socket */
      net->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_CLOSED;
    }
    if ( ( CosmNetRecv( receive_buffer, &bytes, net, 2, wait_time )
      != COSM_PASS ) || ( bytes != 2 ) )
    {
      /* Didn't receive 2 bytes before timeout or other problem */
      /* Make sure the socket is closed and return */
      if ( net->status != COSM_NET_STATUS_CLOSED )
      {
        CosmNetClose( net );
      }
      return COSM_NET_ERROR_CLOSED;
    }
    if ( (u8) receive_buffer[1] == 0xFF )
    {
      /* Username/password authentication method refused */
      /* MUST close the connection */
      CosmNetClose( net );
      net->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_FIREPASS;
    }

    if ( receive_buffer[1] == 0x02 )
    {
      /* Username/password authentication method */
      send_buffer[0] = 0x01;
      send_buffer[1] = (u8) CosmStrBytes( username );
      CosmMemCopy( &send_buffer[2], username, CosmStrBytes( username ) );
      send_buffer_length = (u8) ( CosmStrBytes( username ) + 2 );
      send_buffer[ send_buffer_length ] =
        (u8) CosmStrBytes( password );
      CosmMemCopy( &send_buffer[send_buffer_length + 1], password,
        CosmStrBytes( password ) );
      send_buffer_length = (u8) ( send_buffer_length +
      CosmStrBytes( password ) + 1 );

      if ( CosmNetSend( net, &bytes, send_buffer, send_buffer_length )
        != COSM_PASS )
      {
        /* Unable to send through the socket */
        net->status = COSM_NET_STATUS_CLOSED;
        return COSM_NET_ERROR_CLOSED;
      }
      if ( ( CosmNetRecv( receive_buffer, &bytes, net, 2, wait_time )
        != COSM_PASS ) || ( bytes != 2 ) )
      {
        /* Didn't receive 2 bytes before timeout or other problem */
        /* Make sure the socket is closed and return */
        if ( net->status != COSM_NET_STATUS_CLOSED )
        {
          CosmNetClose( net );
        }
        return COSM_NET_ERROR_CLOSED;
      }
      if ( receive_buffer[1] != 0x00 )
      {
        /* Invalid username or password */
        /* server MUST close the connection */
        CosmNetClose( net );
        return COSM_NET_ERROR_FIREPASS;
      }
      /* Username/password authenticated */
    }
    else if ( receive_buffer[1] != 0x00 )
    {
      /* Error, answer not \0x00 'no authentication necessary' */
      /* nor \0x02 'username/password authentication method'  */
      CosmNetClose( net );
      return COSM_NET_ERROR_FIREPASS;
    }

    /* It's now time to connect to the remote host */
    send_buffer[0] = 0x05;
    send_buffer[1] = 0x01;
    send_buffer[2] = 0x00;
    send_buffer[3] = 0x01;
    /* !!! use load */
    temp_u32_ptr = (u32 *) &send_buffer[4];
    *temp_u32_ptr = htonl( host->ip.v4 );
    temp_u16_ptr = (u16 *) &send_buffer[8];
    *temp_u16_ptr = htons( (u16) host->port );
    if ( CosmNetSend( net, &bytes, send_buffer, 10 ) != COSM_PASS )
    {
      /* Unable to send through the socket */
      net->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_CLOSED;
    }
    if ( ( CosmNetRecv( receive_buffer, &bytes, net, 10, wait_time )
      != COSM_PASS ) || ( bytes != 10 ) )
    {
      /* Didn't receive 10 bytes before timeout or other problem */
      /* Make sure the socket is closed and return */
      if ( net->status != COSM_NET_STATUS_CLOSED )
      {
        CosmNetClose( net );
      }
      return COSM_NET_ERROR_CLOSED;
    }
    if ( receive_buffer[1] != 0x00 )
    {
      /* Connection to host failed */
      /* The server WILL close the connection shortly */
      CosmNetClose( net );
      return COSM_NET_ERROR_ADDRESS;
    }
    /* The connection has been established */
  }
  else
  {
    /* Using a socks4 server */
    send_buffer[0] = 0x04;
    send_buffer[1] = 0x01;
    /* !!! use load */
    temp_u16_ptr = (u16 *) &send_buffer[2];
    *temp_u16_ptr = htons( (u16) host->port );
    temp_u32_ptr = (u32 *) &send_buffer[4];
    *temp_u32_ptr = htonl( host->ip.v4 );
    CosmStrCopy( &send_buffer[8], username, CosmStrBytes( username ) + 2 );
    if ( CosmNetSend( net, &bytes, send_buffer,
      ( CosmStrBytes( username ) + 9 ) ) != COSM_PASS )
    {
      /* Unable to send through the socket */
      net->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_CLOSED;
    }
    if ( ( CosmNetRecv( receive_buffer, &bytes, net, 8, wait_time )
      != COSM_PASS ) || ( bytes != 8 ) )
    {
      /* Didn't receive 8 bytes before timeout or other problem */
      /* Make sure the socket is closed and return */
      if ( net->status != COSM_NET_STATUS_CLOSED )
      {
        CosmNetClose( net );
      }
      return COSM_NET_ERROR_CLOSED;
    }
    if ( ( receive_buffer[1] == 92 ) || ( receive_buffer[1] == 93 ) )
    {
      /* Connection rejected */
      CosmNetClose( net );
      return COSM_NET_ERROR_FIREPASS;
    }
    if ( receive_buffer[1] != 90 )
    {
      /* Connection rejected or failed */
      CosmNetClose( net );
      return COSM_NET_ERROR_ADDRESS;
    }
    /* The connection has been established */
  }

  net->addr = *host;
  net->mode = COSM_NET_MODE_TCP;
  net->status = COSM_NET_STATUS_OPEN;
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetSend( cosm_NET * net, u32 * bytes_sent, const void * data,
  u32 length )
{
#if ( defined( NO_NETWORKING ) )
  *bytes_sent = 0;
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor;
  int result;

  *bytes_sent = 0;

  if ( ( net == NULL ) || ( bytes_sent == NULL )
    || ( data == NULL ) || ( length == 0 ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( net->status != COSM_NET_STATUS_OPEN )
  {
    return COSM_NET_ERROR_CLOSED;
  }

  if ( net->mode != COSM_NET_MODE_TCP )
  {
    return COSM_NET_ERROR_MODE;
  }

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  result = send( socket_descriptor, (const char *) data, length, 0 );

  if ( result == -1 )
  {
    /* Connection has been closed, or other fatal error */
    Cosm_NetClose( net );
    return COSM_NET_ERROR_CLOSED;
  }
#if ( defined( NET_LOG_PACKETS ) )
  Cosm_NetLogPacket( &net->host, "TCP Send:", buffer, result );
#endif
  *bytes_sent = result;

  /* Complete buffer sent */
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetRecv( void * buffer, u32 * bytes_received, cosm_NET * net,
  u32 length, u32 wait_ms )
{
#if ( defined( NO_NETWORKING ) )
  *bytes_received = 0;
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor, flags, result;
  struct timeval select_time;
  fd_set descriptors_settings;
  u8 * data;
  cosmtime time_called, time_now, time_elapsed;
  u64 tmp_u64;
  s32 error;

  *bytes_received = 0;

  if ( ( net == NULL ) || ( bytes_received == NULL )
    || ( buffer == NULL ) || ( length == 0 ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( net->status != COSM_NET_STATUS_OPEN )
  {
    return COSM_NET_ERROR_CLOSED;
  }

  if ( net->mode != COSM_NET_MODE_TCP )
  {
    return COSM_NET_ERROR_MODE;
  }

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  data = (u8 *) buffer;
  flags = 0;
  FD_ZERO( &descriptors_settings );
  FD_SET( socket_descriptor, &descriptors_settings );

  /* The timeval struct gets modified by select, so must be reset each time */
  if ( CosmSystemClock( &time_called ) != COSM_PASS )
  {
    /* Can't get local time */
    return COSM_NET_ERROR_FATAL;
  }

  while ( *bytes_received < length )
  {
    if ( CosmSystemClock( &time_now ) != COSM_PASS )
    {
      /* Can't get local time */
      return COSM_NET_ERROR_FATAL;
    }

    /* Lets fill that select_time for the remaining time to wait */
    time_elapsed = CosmS128Sub( time_now, time_called );
    tmp_u64 = 0x000010C6F7A0B5EDLL;
    select_time.tv_sec = ( wait_ms / 1000 ) - (s32) time_elapsed.hi;
    select_time.tv_usec = ( ( wait_ms % 1000 ) * 1000 ) -
      (u32) ( time_elapsed.lo / tmp_u64 );

    if ( select_time.tv_usec < 0 )
    {
      select_time.tv_usec = select_time.tv_usec + 1000000;
      select_time.tv_sec--;
    }
    if ( select_time.tv_sec < 0 )
    {
      /* Time is now elapsed, just take what's in the buffer and exit */
      select_time.tv_sec = 0;
      select_time.tv_usec = 0;
    }

    result = select( socket_descriptor + 1, &descriptors_settings,
      (fd_set *) NULL, (fd_set *) NULL, &select_time );
    if ( result < 1 )
    {
      if ( result == -1 )
      {
        /* Error with select */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      else
      {
        /* Timeout */
        return COSM_PASS;
      }
      return error;
    }
    result = recv( socket_descriptor, (char *) &data[*bytes_received],
      length - *bytes_received, flags );
    if ( result < 1 )
    {
      if ( result == 0 )
      {
        /* Connection terminated gracefully */
        Cosm_NetClose( net );
        error = COSM_NET_ERROR_CLOSED;
      }
      else
      {
        /* Error while receiving data, connection dead */
        Cosm_NetClose( net );
        error = COSM_NET_ERROR_SOCKET;
      }
      return error;
    }
#if ( defined( NET_LOG_PACKETS ) )
    Cosm_NetLogPacket( &net->host, "TCP Recv:",
      &data[*bytes_received], result );
#endif
    *bytes_received += result;
  }

  /* Received full buffer */
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetSendUDP( cosm_NET * net, const cosm_NET_ADDR * addr,
  const void * data, u32 length )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor;
  int result;
  int addr_length;
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    return COSM_NET_ERROR_NO_NET;
  }

  if ( net == NULL )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( ( data == NULL ) || ( length == 0 ) )
  {
    /* no data to send is OK */
    return COSM_PASS;
  }

  if ( net->status != COSM_NET_STATUS_OPEN )
  {
    return COSM_NET_ERROR_CLOSED;
  }

  if ( net->mode != COSM_NET_MODE_UDP )
  {
    return COSM_NET_ERROR_MODE;
  }

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  if ( addr->type == COSM_NET_IPV4 )
  {
    addr_length = sizeof( addr4 );
    CosmMemSet( &addr4, sizeof( addr4 ), 0 );

    /* set destination */
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons( (u16) addr->port );
    CosmU32Save( &addr4.sin_addr, &addr->ip.v4 );

    /* send the UDP packet, cross fingers :) */
    result = sendto( socket_descriptor, (const char *) data, length, 0,
      (struct sockaddr *) &addr4, addr_length );
  }
  else if ( addr->type == COSM_NET_IPV6 )
  {
    addr_length = sizeof( addr6 );
    CosmMemSet( &addr6, sizeof( addr6 ), 0 );

    /* set destination */
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons( (u16) addr->port );
    CosmU128Save( &addr6.sin6_addr, &addr->ip.v6 );

    /* send the UDP packet, cross fingers :) */
    result = sendto( socket_descriptor, (const char *) data, length, 0,
      (struct sockaddr *) &addr6, addr_length );
  }
  else
  {
    return COSM_NET_ERROR_ADDRTYPE;
  }

  if ( result == -1 )
  {
    /* connection has been closed, or other error */
    close( socket_descriptor );
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_CLOSED;
  }
#if ( defined( NET_LOG_PACKETS ) )
  Cosm_NetLogPacket( &net->host, "UDP Send:", (u8 *) data, result );
#endif

  return COSM_PASS;
#endif
}

s32 CosmNetRecvUDP( void * buffer, u32 * bytes_read, cosm_NET_ADDR * from,
  cosm_NET * net, u32 length, cosm_NET_ACL * acl, u32 wait_ms )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor;
  int received;
  s32 result;
  struct timeval select_time;
  fd_set descriptors;
  cosmtime time_called, time_now, time_elapsed;
  u64 tmp_u64;
  struct sockaddr_in client_addr;
  unsigned int client_addr_len;
  s32 error;

  *bytes_read = 0;

  if ( ( buffer == NULL ) || ( bytes_read == NULL )
    || ( from == NULL ) || ( net == NULL ) || ( length == 0 ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( net->status != COSM_NET_STATUS_OPEN )
  {
    return COSM_NET_ERROR_CLOSED;
  }

  if ( net->mode != COSM_NET_MODE_UDP )
  {
    return COSM_NET_ERROR_MODE;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_NO_NET;
  }

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  if ( CosmSystemClock( &time_called ) != COSM_PASS )
  {
    /* can't get local time */
    return COSM_NET_ERROR_FATAL;
  }

  received = 0;
  client_addr_len = sizeof( client_addr );

  while ( received == 0 )
  {
    /* timeval struct must be reset each time we fail to match the ACL */
    if ( CosmSystemClock( &time_now ) != COSM_PASS )
    {
      /* can't get local time */
      return COSM_NET_ERROR_TIMEOUT;
    }

    /* fill that select_time for the remaining time to wait */
    time_elapsed = CosmS128Sub( time_now, time_called );
    tmp_u64 = 0x000010C6F7A0B5EDLL;
    select_time.tv_sec = ( wait_ms / 1000 ) - (s32) time_elapsed.hi;
    select_time.tv_usec = ( ( wait_ms % 1000 ) * 1000 ) -
      (u32) ( time_elapsed.lo / tmp_u64 );

    if ( select_time.tv_usec < 0 )
    {
      select_time.tv_usec = select_time.tv_usec + 1000000;
      select_time.tv_sec--;
    }
    if ( select_time.tv_sec < 0 )
    {
      /* time is now elapsed, just take what's in the buffer and exit */
      select_time.tv_sec = 0;
      select_time.tv_usec = 0;
    }

    FD_ZERO( &descriptors );
    FD_SET( socket_descriptor, &descriptors );
    result = select( socket_descriptor + 1, &descriptors,
      (fd_set *) NULL, (fd_set *) NULL, &select_time );
    if ( result < 1 )
    {
      if ( result == -1 )
      {
        /* error with select */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      else
      {
        /* timeout */
        return COSM_PASS;
      }
    }

    received = recvfrom( socket_descriptor, (char *) buffer,
      length - received, 0, (struct sockaddr *) &client_addr,
      &client_addr_len );

    if ( received < 1 )
    {
      if ( received == 0 )
      {
        /* connection terminated */
        net->status = COSM_NET_STATUS_CLOSED;
        error = COSM_NET_ERROR_CLOSED;
      }
      else
      {
        /* error while receiving data */
        error = COSM_NET_ERROR_SOCKET;
      }
      return error;
    }

    /* set from !!! */

    /* only accept this data if host passes acl masks */
    /*
    if ( CosmNetACLCheck( acl, ntohl( client_addr.sin_addr.s_addr ) )
      != COSM_NET_ALLOW )
    {
      received = 0;
    }
    */
#if ( defined( NET_LOG_PACKETS ) )
    Cosm_NetLogPacket( &net->host, "UDP Recv:", buffer, received );
#endif
  }

  return COSM_PASS;
#endif
}

s32 CosmNetListen( cosm_NET * net, const cosm_NET_ADDR * addr, u32 mode,
  u32 queue )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor;
  unsigned int addr_length;
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;

#if 0
#if ( ( defined( SO_REUSEADDR ) ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) ) )
  int option;
#endif
#endif

  if ( net == NULL )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( ( mode != COSM_NET_MODE_TCP ) && ( mode != COSM_NET_MODE_UDP ) )
  {
    return COSM_NET_ERROR_MODE;
  }

  if ( net->status != COSM_NET_STATUS_CLOSED )
  {
    return COSM_NET_ERROR_ORDER;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_NO_NET;
  }

  if ( addr->type == COSM_NET_IPV4 )
  {
    if ( mode == COSM_NET_MODE_UDP )
    {
      socket_descriptor = (int) socket( PF_INET, SOCK_DGRAM, 0 );
    }
    else
    {
      socket_descriptor = (int) socket( PF_INET, SOCK_STREAM, 0 );
    }
  }
  else if ( addr->type == COSM_NET_IPV6 )
  {
    if ( mode == COSM_NET_MODE_UDP )
    {
      socket_descriptor = (int) socket( PF_INET6, SOCK_DGRAM, 0 );
    }
    else
    {
      socket_descriptor = (int) socket( PF_INET6, SOCK_STREAM, 0 );
    }
  }
  else
  {
    return COSM_NET_ERROR_ADDRTYPE;
  }

  if ( socket_descriptor == -1 )
  {
    /* unable to open the socket */
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_SOCKET;
  }

#if 0
#if ( ( defined( SO_REUSEADDR ) ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) ) )
  /*
    This allows us to reuse the same UNIX port if it's still closing.
    Win32 (others?) doesn't do this correctly since it does SO_REUSEPORT as
    well, which will not give an error if we have an open listener already.
  */
  option = 1;
  if ( setsockopt( socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
    (void *) &option, sizeof( option ) ) == -1 )
  {
    /* unable to set the socket REUSEADDR option */
    Cosm_NetClose( net );
    return COSM_NET_ERROR_SOCKET;
  }
#endif
#endif

  if ( addr->type == COSM_NET_IPV4 )
  {
    addr_length = sizeof( addr4 );
    CosmMemSet( &addr4, sizeof( addr4 ), 0 );
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons( (u16) addr->port );
    CosmU32Save( &addr4.sin_addr, &addr->ip.v4 );

    if ( bind( socket_descriptor, (struct sockaddr *) &addr4,
      addr_length ) == -1 )
    {
      /* unable to bind to the socket */
      Cosm_NetClose( net );
      return COSM_NET_ERROR_ADDRESS;
    }
  }
  else /* IPv6 */
  {
    addr_length = sizeof( addr6 );
    CosmMemSet( &addr6, sizeof( addr6 ), 0 );
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons( (u16) addr->port );
    CosmU128Save( &addr6.sin6_addr, &addr->ip.v6 );

    if ( bind( socket_descriptor, (struct sockaddr *) &addr6,
      addr_length ) == -1 )
    {
      /* unable to bind to the socket */
      Cosm_NetClose( net );
      return COSM_NET_ERROR_ADDRESS;
    }
  }

  /* For UDP connection we're done now */
  if ( mode == COSM_NET_MODE_UDP )
  {
    return COSM_PASS;
  }

  if ( listen( socket_descriptor, queue ) == -1 )
  {
    /* unable to listen to the socket */
    Cosm_NetClose( net );
    return COSM_NET_ERROR_SOCKET;
  }

  if ( addr->port == 0 )
  {
    if ( addr->type == COSM_NET_IPV4 )
    {
      if ( getsockname( socket_descriptor, (struct sockaddr *)
        &addr4, &addr_length ) == -1 )
      {
        /* unable to get the port the socket is listenning on */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      net->my_addr.port = ntohs( addr4.sin_port );
    }
    else /* IPv6 */
    {
      if ( getsockname( socket_descriptor, (struct sockaddr *)
        &addr6, &addr_length ) == -1 )
      {
        /* unable to get the port the socket is listenning on */
        Cosm_NetClose( net );
        return COSM_NET_ERROR_SOCKET;
      }
      net->my_addr.port = ntohs( addr6.sin6_port );
    }
  }
  else
  {
    net->my_addr.port = addr->port;
  }

  net->my_addr.type = addr->type;
  net->my_addr.ip.v6 = addr->ip.v6; /* ip.v6 copies both */

  net->handle = (u64) socket_descriptor;

  net->mode = mode;
  net->status = COSM_NET_STATUS_LISTEN;
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetAccept( cosm_NET * new_connection, cosm_NET * net,
  cosm_NET_ACL * acl, u32 wait )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor, new_socket_descriptor;
  struct sockaddr_in client_address, local_address;
  unsigned int client_address_length, local_address_length;
  fd_set descriptors_settings;
  struct timeval select_time;
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  unsigned long flags;
#else
  int flags;
#endif
  int result;
  s32 acl_test_result;
  s32 error;

  if ( ( net == NULL ) || ( new_connection == NULL ) )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( net->status != COSM_NET_STATUS_LISTEN )
  {
    return COSM_NET_ERROR_ORDER;
  }

  /* CosmNetAccept is not valid for UDP connections */
  if ( net->mode != COSM_NET_MODE_TCP )
  {
    return COSM_NET_ERROR_MODE;
  }

  flags = 0;

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  if ( wait == COSM_NET_ACCEPT_NOWAIT )
  {
    /* We want select to return immediately */
    select_time.tv_sec = 0;
    select_time.tv_usec = 0;

    FD_ZERO( &descriptors_settings );
    FD_SET( socket_descriptor, &descriptors_settings );
    result = select( socket_descriptor + 1, &descriptors_settings,
      (fd_set *) NULL, (fd_set *) NULL, &select_time );

    if ( result < 1 )
    {
      if ( result == -1 )
      {
        /* Error with select */
        error = COSM_NET_ERROR_SOCKET;
      }
      else
      {
        /* No connection waiting */
        error = COSM_NET_ERROR_TIMEOUT;
      }
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return error;
    }

    /* Making sure we won't block on accept() if the connection is lost */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    flags = 1; /* set nonblocking mode */
    if ( ioctlsocket( socket_descriptor, FIONBIO, &flags ) != 0 )
    {
      /* Unable to set flags of the socket to non-blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_SOCKET;
    }
#else
    /* Change the socket's flags - Turn off blocking */
    flags = fcntl( socket_descriptor, F_GETFL, 0 );
    flags = flags | O_NONBLOCK;
    if ( fcntl( socket_descriptor, F_SETFL, flags ) != 0 )
    {
      /* Unable to set flags of the socket to non-blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_SOCKET;
    }
#endif
  }

  client_address_length = sizeof( client_address );

  /* we loop if wait != COSM_NET_ACCEPT_NOWAIT and ACL failed. */
  do
  {
    /* accept() the connection ... */
    new_socket_descriptor = (int) accept( socket_descriptor,
      (struct sockaddr *) &client_address, &client_address_length );

    /* if we connected ... */
    if ( new_socket_descriptor == -1 )
    {
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_SOCKET;
    }
    else
    {
      new_connection->addr.type = COSM_NET_IPV4;
      new_connection->addr.ip.v4
        = ntohl( client_address.sin_addr.s_addr );
      new_connection->addr.port = ntohs( client_address.sin_port );

      /* Make sure the host is allowed by ACL masks */
      acl_test_result = CosmNetACLCheck( acl, &new_connection->addr );
      if ( acl_test_result == COSM_NET_DENY )
      {
        close( new_socket_descriptor );
      }
    }
  } while ( ( acl_test_result == COSM_NET_DENY )
    && ( wait != COSM_NET_ACCEPT_NOWAIT ) );

  new_connection->handle = (u64) new_socket_descriptor;

  if ( wait == COSM_NET_ACCEPT_NOWAIT )
  {
    /*
      The old socket need to be set blocking again in case the next
      call to CosmNetListen() is with COSM_NET_ACCEPT_WAIT
    */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    flags = 0; /* (re)set blocking mode */
    if ( ioctlsocket( socket_descriptor, FIONBIO, &flags ) == -1 )
    {
      /* Unable to reset flags of the listenning socket to blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_SOCKET;
    }
    if ( new_socket_descriptor > 0 )
    {
      if ( ioctlsocket( new_socket_descriptor, FIONBIO, &flags ) == -1 )
      {
        /* Unable to set flags of the new socket to blocking */
        new_connection->status = COSM_NET_STATUS_CLOSED;
        return COSM_NET_ERROR_SOCKET;
      }
    }
#else
    flags = flags & ~O_NONBLOCK;
    if ( fcntl( socket_descriptor, F_SETFL, flags ) == -1 )
    {
      /* Unable to reset flags of the listenning socket to blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_SOCKET;
      /* The new connection may still be open, lets continue */
    }
#endif
  }

  /* We have nothing more to do with net */

  /* check that we got a connection */
  if ( new_socket_descriptor == -1 )
  {
    /* Unable to accept a client - some error ? */
    /* On NOWAIT, may be a lost connection between select() and accept() */
    new_connection->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_CLOSED;
  }

  if ( wait == COSM_NET_ACCEPT_NOWAIT )
  {
    /* The new socket must also be set to blocking */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    flags = 0;
    if ( ioctlsocket( new_socket_descriptor, FIONBIO, &flags ) == -1 )
    {
      /* Unable to set flags of the new socket to blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_CLOSED;
    }
#else
    /* Change the new socket's flags - Turn on blocking */
    flags = fcntl( new_socket_descriptor, F_GETFL, 0 );
    flags = flags & ~O_NONBLOCK;
    if ( fcntl( new_socket_descriptor, F_SETFL, flags ) == -1 )
    {
      /* Unable to set flags of the new socket to blocking */
      new_connection->status = COSM_NET_STATUS_CLOSED;
      return COSM_NET_ERROR_CLOSED;
    }
#endif
  }

  /* verify which address the client has connected to */
  local_address_length = sizeof( local_address );
  if ( getsockname( new_socket_descriptor, (struct sockaddr *)
    &local_address, &local_address_length ) == -1 )
  {
    /* unable to get the local address and port of the socket */
    new_connection->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_CLOSED;
  }

  /* Filling the new connection cosm_NET struct */
  new_connection->my_addr.type = COSM_NET_IPV4;
  new_connection->my_addr.ip.v4 = ntohl( local_address.sin_addr.s_addr );
  new_connection->my_addr.port = ntohs( local_address.sin_port );
  new_connection->mode = COSM_NET_MODE_TCP;

  new_connection->status = COSM_NET_STATUS_OPEN;
  return COSM_PASS;
#endif /* NO_NETWORKING */
}

s32 CosmNetClose( cosm_NET * net )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  s32 error;

  if ( net == NULL )
  {
    return COSM_NET_ERROR_PARAM;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    net->status = COSM_NET_STATUS_CLOSED;
    return COSM_NET_ERROR_NO_NET;
  }

  error = Cosm_NetClose( net );

  return error;
#endif /* NO_NETWORKING */
}

u32 CosmNetDNS( cosm_NET_ADDR * addr, u32 count, ascii * name )
{
#if ( defined( NO_NETWORKING ) )
  return 0;
#else
  struct addrinfo hint, * entry, * addr_list;
  u32 found;

  if ( ( addr == NULL ) || ( count == 0 ) || ( name == NULL ) )
  {
    return 0;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    return 0;
  }

  /* hint for just 1 version of each IP */
  CosmMemSet( &hint, sizeof( hint ), 0 );
  hint.ai_family = PF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  if ( getaddrinfo( name, NULL, &hint, &addr_list ) != 0 )
  {
    return 0;
  }

  found = 0;
  entry = addr_list;
  while ( ( entry != NULL ) && ( found < count ) )
  {
    if ( ( entry->ai_family == PF_INET )
      && ( ( (struct sockaddr_in *) entry->ai_addr )->sin_family
      == AF_INET ) )
    {
      /* IPv4 */
      addr[found].type = COSM_NET_IPV4;
      addr[found].port = 0;
      /* set the ipv6 to avoid uninitilized memory warnings */
      addr[found].ip.v6.hi = 0;
      addr[found].ip.v6.lo = 0;
      CosmU32Load( &addr[found].ip.v4,
        &( ( (struct sockaddr_in *) entry->ai_addr )->sin_addr ) );
      found++;
    }
    else if ( ( entry->ai_family == PF_INET6 )
      && ( ( (struct sockaddr_in6 *) entry->ai_addr )->sin6_family
      == AF_INET6 ) )
    {
      /* IPv6 */
      addr[found].type = COSM_NET_IPV6;
      addr[found].port = 0;
      CosmU128Load( &addr[found].ip.v6,
        &( ( (struct sockaddr_in6 *) entry->ai_addr )->sin6_addr ) );
      found++;
    }

    entry = entry->ai_next;
  }

  freeaddrinfo( addr_list );

  return found;
#endif /* NO_NETWORKING */
}

s32 CosmNetRevDNS( cosm_NET_HOSTNAME * name, const cosm_NET_ADDR * addr )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_FAIL;
#else
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
  int addr_length;

  if ( ( name == NULL ) || ( addr == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    return COSM_FAIL;
  }

  if ( addr->type == COSM_NET_IPV4 )
  {
    addr_length = sizeof( addr4 );
    CosmMemSet( &addr4, sizeof( addr4 ), 0 );
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons( (u16) addr->port );
    CosmU32Save( &addr4.sin_addr, &addr->ip.v4 );

    if ( getnameinfo( (struct sockaddr *) &addr4, addr_length,
      (char *) name, COSM_NET_MAX_HOSTNAME, NULL, 0, 0 ) != 0 )
    {
      return COSM_FAIL;
    }
  }
  else if ( addr->type == COSM_NET_IPV6 )
  {
    addr_length = sizeof( addr6 );
    CosmMemSet( &addr6, sizeof( addr6 ), 0 );
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons( (u16) addr->port );
    CosmU128Save( &addr6.sin6_addr, &addr->ip.v6 );

    if ( getnameinfo( (struct sockaddr *) &addr6, addr_length,
      (char *) name, COSM_NET_MAX_HOSTNAME, NULL, 0, 0 ) != 0 )
    {
      return COSM_FAIL;
    }
  }
  else
  {
    return COSM_FAIL;
  }

  return COSM_PASS;
#endif /* NO_NETWORKING */
}

u32 CosmNetMyIP( cosm_NET_ADDR * addr, u32 count )
{
  u32 found = 0;

#if ( defined( NO_NETWORKING ) )
  return 0;
#elif ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  IP_ADAPTER_ADDRESSES * adapter, * current_addr;
  IP_ADAPTER_UNICAST_ADDRESS * unicast;
  u32 length;
  s32 error;

  /* first get a count */
  length = sizeof( IP_ADAPTER_ADDRESSES );
  adapter = CosmMemAlloc( length );
  error = GetAdaptersAddresses( AF_INET,
    GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST, NULL,
    adapter, &length );
  if ( ERROR_BUFFER_OVERFLOW == error )
  {
    CosmMemFree( adapter );
    adapter = CosmMemAlloc( length );
    error = ERROR_SUCCESS;
  }
  
  if ( ERROR_SUCCESS == error )
  {
    /* get addresses */
    if ( ERROR_SUCCESS == GetAdaptersAddresses( AF_INET,
      GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST, NULL,
      adapter, &length ) )
    {
      current_addr = adapter;
      while ( ( current_addr ) && ( found < count ) )
      {
        unicast = current_addr->FirstUnicastAddress;
        while ( ( unicast ) && ( found < count ) )
        {
          addr[found].type = COSM_NET_IPV4;
          addr[found].port = 0;
          CosmU32Load( &addr[found].ip.v4, &( ( (struct sockaddr_in *)
            unicast->Address.lpSockaddr)->sin_addr ) );
          found++;
        
          unicast = unicast->Next;
        }
        current_addr = current_addr->Next;
      }
    }
  }
  CosmMemFree( adapter );

  /* first get a count */
  length = sizeof( IP_ADAPTER_ADDRESSES );
  adapter = CosmMemAlloc( length );
  error = GetAdaptersAddresses( AF_INET6,
    GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST, NULL,
    adapter, &length );
  if ( ERROR_BUFFER_OVERFLOW == error )
  {
    CosmMemFree( adapter );
    adapter = CosmMemAlloc( length );
    error = ERROR_SUCCESS;
  }

  if ( ERROR_SUCCESS == error )
  {
    /* get addresses */
    if ( ERROR_SUCCESS == GetAdaptersAddresses( AF_INET6,
      GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST, NULL,
      adapter, &length ) )
    {
      current_addr = adapter;
      while ( ( current_addr ) && ( found < count ) )
      {
        unicast = current_addr->FirstUnicastAddress;
        while ( ( unicast ) && ( found < count ) )
        {
          addr[found].type = COSM_NET_IPV6;
          addr[found].port = 0;
          CosmU128Load( &addr[found].ip.v6, &( ( (struct sockaddr_in6 *)
            unicast->Address.lpSockaddr)->sin6_addr ) );
          found++;
        
          unicast = unicast->Next;
        }
        current_addr = current_addr->Next;
      }
    }
  }
  CosmMemFree( adapter );
#elif ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_MACOSX ) )
  /* platforms with getifaddrs */
  struct ifaddrs * head, * current;

  if ( ( NULL == addr ) || ( 0 == count ) )
  {
    return 0;
  }

  if ( -1 == getifaddrs( &head ) )
  {
    return 0;
  }

  found = 0;
  current = head;
  while ( NULL != current )
  {
    if ( current->ifa_flags & IFF_UP )
    {
      if ( AF_INET == current->ifa_addr->sa_family )
      {
        addr[found].type = COSM_NET_IPV4;
        addr[found].port = 0;
        CosmU32Load( &addr[found].ip.v4,
          &( ( (struct sockaddr_in *) current->ifa_addr )->sin_addr ) );
        found++;
      }
      else if ( AF_INET6 == current->ifa_addr->sa_family )
      {
        addr[found].type = COSM_NET_IPV6;
        addr[found].port = 0;
        CosmU128Load( &addr[found].ip.v6,
          &( ( (struct sockaddr_in6 *) current->ifa_addr )->sin6_addr ) );
        found++;
      }
    }
    current = current->ifa_next;
  }
  
  freeifaddrs( head );
#else /* no getifaddrs or ipv6 */
  cosm_NET_HOSTNAME my_name;

  if ( ( addr == NULL ) || ( count == 0 ) )
  {
    return 0;
  }

  if ( Cosm_NetGlobalInit() != COSM_PASS )
  {
    return 0;
  }

  if ( gethostname( (char *) my_name, sizeof( my_name ) ) == -1 )
  {
    /* Can't get local hostname */
    return 0;
  }

  if ( ( found = CosmNetDNS( addr, count, my_name ) ) == 0 )
  {
    /* Can't get IP matching local hostname */
    return 0;
  }
#endif

  return found;
}

s32 CosmNetACLAdd( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr,
  u32 mask_bits, u32 permission, cosmtime expires )
{
  u32 i;
  u64 size;
  void * mem;

  if ( ( acl == NULL ) || ( addr == NULL ) || ( mask_bits > 128 )
    || ( ( permission != COSM_NET_ALLOW ) && ( permission != COSM_NET_DENY ) )
    || ( ( addr->type != COSM_NET_IPV4 ) && ( addr->type != COSM_NET_IPV6 ) ) )
  {
    return COSM_FAIL;
  }

  CosmMutexLock( &acl->lock, COSM_MUTEX_WAIT );

  if ( ( acl->count + 1 ) > acl->length )
  {
    if ( acl->length == 0 )
    {
      /* 16 at a time */
      size = sizeof( cosm_NET_ACL_ENTRY ) << 4;
      if ( ( mem = CosmMemAlloc( size ) ) == NULL )
      {
        CosmMutexUnlock( &acl->lock );
        return COSM_FAIL;
      }
      acl->list = mem;
      acl->length = 16;
    }
    else
    {
      /* 16 at a time */
      size = sizeof( cosm_NET_ACL_ENTRY ) * ( (u64) acl->length + 16LL );
      if ( ( mem = CosmMemRealloc( acl->list, size ) ) == NULL )
      {
        CosmMutexUnlock( &acl->lock );
        return COSM_FAIL;
      }
      acl->list = mem;
      acl->length += 16;
    }
  }

  i = acl->count++;
  acl->list[i].expires = expires;
  acl->list[i].addr = *addr;
  acl->list[i].mask.type = addr->type;
  acl->list[i].mask.port = addr->port;
  if ( addr->type == COSM_NET_IPV4 )
  {
    acl->list[i].mask.ip.v4 = ( 0xFFFFFFFF << ( 32 - mask_bits ) );
  }
  else
  {
    _COSM_SET128( acl->list[i].mask.ip.v6, FFFFFFFFFFFFFFFF,
      FFFFFFFFFFFFFFFF );
    acl->list[i].mask.ip.v6
      = CosmU128Lsh( acl->list[i].mask.ip.v6, 128 - mask_bits );
  }
  acl->list[i].permission = permission;

  CosmMutexUnlock( &acl->lock );
  return COSM_PASS;
}

s32 CosmNetACLDelete( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr,
  u32 mask_bits )
{
  u32 i, j;
  cosm_NET_ACL_ENTRY * list;
  cosm_NET_ADDR mask;

  if ( ( acl == NULL ) || ( addr == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( acl->count == 0 )
  {
    return COSM_PASS;
  }

  CosmMutexLock( &acl->lock, COSM_MUTEX_WAIT );

  list = acl->list;

  i = 0;
  mask.ip.v4 = ( 0xFFFFFFFF << ( 32 - mask_bits ) );
  do
  {
    if ( ( addr->ip.v4 == list[i].addr.ip.v4 )
      && ( mask.ip.v4 == list[i].mask.ip.v4 ) )
    {
      for ( j = i ; j < ( acl->count - 1 ) ; j++ )
      {
        list[j] = list[j+1];
      }
      acl->count--;
    }
    else
    {
      i++;
    }
  } while ( i < acl->count );

  CosmMutexUnlock( &acl->lock );
  return COSM_PASS;
}

s32 CosmNetACLCheck( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr )
{
  u32 i, j;
  s32 result;
  cosm_NET_ACL_ENTRY * list;
  cosmtime time;

  if ( acl == NULL )
  {
    return COSM_NET_ALLOW;
  }

  if ( acl->count == 0 )
  {
    return COSM_NET_ALLOW;
  }

  list = acl->list;
  result = COSM_NET_ALLOW;

  if ( CosmSystemClock( &time ) == COSM_PASS )
  {
    i = 0;

    CosmMutexLock( &acl->lock, COSM_MUTEX_WAIT );

    do
    {
      /* expired? */
      if ( CosmS128Gt( list[i].expires, time ) )
      {
        for ( j = i ; j < ( acl->count - 1 ) ; j++ )
        {
          list[j] = list[j+1];
        }
        acl->count--;
      }
      else
      {
        /* match? */
        if ( ( addr->ip.v4 & list[i].mask.ip.v4 )
          == ( list[i].addr.ip.v4 & list[i].mask.ip.v4 ) )
        {
          result = list[i].permission;
        }
        i++;
      }
    } while ( i < acl->count );

    CosmMutexUnlock( &acl->lock );
  }

  return result;
}

void CosmNetACLFree( cosm_NET_ACL * acl )
{
  if ( acl == NULL )
  {
    return;
  }

  CosmMutexLock( &acl->lock, COSM_MUTEX_WAIT );

  acl->length = 0;
  acl->count = 0;

  CosmMemFree( acl->list );

  CosmMutexUnlock( &acl->lock );
}

s32 Cosm_NetGlobalInit( void )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_FAIL;
#else
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  static WSADATA net_wsadata;

  if ( ( __cosm_net_global_init == 0 )
    && ( WSAStartup( MAKEWORD( 2, 2 ), &net_wsadata ) != 0 ) )
  {
    return COSM_FAIL;
  }
#else
  if ( __cosm_net_global_init == 0 )
  {
#if ( defined( SIGPIPE ) )
    /*
      SIGPIPE must be ignored or network functions kill the program rather
      then just failing gracefully.
    */
    signal( SIGPIPE, SIG_IGN );
#endif
  }
#endif /* OS */

  __cosm_net_global_init = 1;
  return COSM_PASS;

#endif /* NO_NETWORKING */
}

s32 Cosm_NetClose( cosm_NET * net )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_NET_ERROR_NO_NET;
#else
  int socket_descriptor;
  s32 error;

  /* make sure it's not already closed */
  if ( net->status == COSM_NET_STATUS_CLOSED )
  {
    return COSM_PASS;
  }

#if ( defined( CPU_64BIT ) )
  socket_descriptor = net->handle;
#else
  socket_descriptor = (u32) net->handle;
#endif

  error = COSM_PASS;
  if ( close( socket_descriptor ) != 0 )
  {
    /* Unable to close the socket, general OS error and unrecoverable */
    error = COSM_NET_ERROR_FATAL;
  }

  /* mark socket closed */
  net->status = COSM_NET_STATUS_CLOSED;

  return error;
#endif
}

#if ( defined( NET_LOG_PACKETS ) )
#include "cosm/os_file.h"

Cosm_NetLogPacket( const cosm_NET_ADDR * addr, ascii * tag,
  u8 * data, u32 length )
{
  cosm_FILE file;
  ascii filename[128];
  u32 i;
  u32 line;

  CosmMemSet( &file, sizeof( cosm_FILE ), 0 );

  CosmPrintStr( filename, (u64) 128, "%u-%u-%u-%u-%u.log",
    ( addr->ip.v4 >> 24 ) & 0xFF, ( addr->ip.v4 >> 16 ) & 0xFF,
    ( addr->ip.v4 >> 8 ) & 0xFF, addr->ip.v4 & 0xFF, addr->port );

  if ( CosmFileOpen( &file, filename, COSM_FILE_MODE_CREATE |
    COSM_FILE_MODE_APPEND, COSM_FILE_LOCK_WRITE ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  CosmPrintFile( &file, tag );
  CosmPrintFile( &file, "\n" );

  while ( length > 0 )
  {
    if ( length > 15 )
    {
      line = 16;
    }
    else
    {
      line = length;
    }

    CosmPrintFile( &file, " " );
    for ( i = 0 ; i < line ; i++ )
    {
      CosmPrintFile( &file, " %02X", data[i] );
    }
    for ( i = 0 ; i < ( 16 - line ) ; i++ )
    {
      CosmPrintFile( &file, "  ", data[i] );
    }
    CosmPrintFile( &file, " | " );
    for ( i = 0 ; i < line ; i++ )
    {
      if ( ( data[i] < 0x80 ) && ( data[i] > 0x1F ) )
      {
        CosmPrintFile( &file, "%c", data[i] );
      }
      else
      {
        CosmPrintFile( &file, " ", data[i] );
      }
    }
    data = &data[line];
    length -= line;
    CosmPrintFile( &file, "\n" );
  }

  CosmFileClose( &file );

  return COSM_PASS;
}
#endif

/* Testing */

s32 Cosm_TestOSNet( void )
{
#if ( defined( NO_NETWORKING ) )
  return COSM_PASS;
#else
  cosm_NET netsrv, netsrv1, netsrv2, netclient1, netclient2;
  cosm_NET_ADDR my_addr, addr;
  cosm_NET_ACL net_acl;
  ascii buf1[128], buf2[128];
  /* cosm_NET_HOSTNAME host_name; */
  u32 bytes;
  u32 i;
  u32 wait_time = 250;

  if ( 0 == CosmNetDNS( &my_addr, 1, "127.0.0.1" ) )
  {
    return -1;
  }

  if ( ( ( my_addr.type == COSM_NET_IPV4 )
    && ( my_addr.ip.v4 != 0x7F000001 ) )
    || ( ( my_addr.type == COSM_NET_IPV6 )
    && ( !CosmU128Eq( my_addr.ip.v6, CosmU128U32( 1 ) ) ) )
    || ( my_addr.port != 0 ) )
  {
    return -2;
  }

  CosmMemSet( &netsrv, sizeof( cosm_NET ), 0 );
  CosmMemSet( &netsrv1, sizeof( cosm_NET ), 0 );
  CosmMemSet( &netsrv2, sizeof( cosm_NET ), 0 );
  CosmMemSet( &netclient1, sizeof( cosm_NET ), 0 );
  CosmMemSet( &netclient2, sizeof( cosm_NET ), 0 );
  CosmMemSet( &net_acl, sizeof( cosm_NET_ACL ), 0 );
  CosmMemSet( buf1, sizeof( buf1 ), 0 );
  CosmMemSet( buf2, sizeof( buf2 ), 0 );

  for ( i = 0 ; i < 10 ; i++ )
  {
    buf1[i] = (ascii) i;
  }

  /*  Let the machine choose a free port */
  addr.type = COSM_NET_IPV4;
  addr.ip.v4 = 0;
  addr.port = 0;
  if ( CosmNetListen( &netsrv, &addr, COSM_NET_MODE_TCP, 1  ) != COSM_PASS )
  {
    return -3;
  }

  /* Make sure we can't bind the exact same already bound port */
  addr.type = COSM_NET_IPV4;
  addr.ip.v4 = 0;
  addr.port = netsrv.my_addr.port;
  if ( CosmNetListen( &netsrv1, &addr, COSM_NET_MODE_TCP, 1 )
    == COSM_PASS )
  {
    return -4;
  }

  /* Lets bind to a specific IP */
  addr.type = COSM_NET_IPV4;
  addr.ip.v4 = my_addr.ip.v4;
  addr.port = 0;
  if ( CosmNetListen( &netsrv1, &addr, COSM_NET_MODE_TCP, 1 ) != COSM_PASS )
  {
    return -6;
  }

  /* Unless MyIP is localhost */
#ifdef __FIX_THIS
  if ( netsrv1.addr.ip.v4 != my_addr.ip.v4 )
  {
    /* Connect to our server socket using the wrong IP */
    if ( CosmNetOpen( &netclient1, addr1, netsrv1->my_port, 0,
      COSM_NET_MODE_TCP, 0, 0, NULL ) == COSM_PASS )
    {
      return -7;
    }

    /* Lets bind to the other IP, same port */
    if ( CosmNetListen( &netsrv2, addr1, netsrv1->my_port,
      COSM_NET_MODE_TCP, 1 ) != COSM_PASS )
    {
      return -8;
    }

    /* Lets try to close it */
    if ( CosmNetClose( &netsrv2 ) != COSM_PASS )
    {
      return -9;
    }
  }
#endif

  /* Lets try to close netsrv1 as well */
  if ( CosmNetClose( &netsrv1 ) != COSM_PASS )
  {
    return -10;
  }

  /* Connect to our server socket on localhost. */
  addr.type = my_addr.type;
  addr.ip = my_addr.ip;
  addr.port = netsrv.my_addr.port;
  if ( _COSM_NETOPEN( &netclient1, &addr ) != COSM_PASS )
  {
    return -11;
  }

  CosmYield();
  /* The above connection should be pending. */
  if ( CosmNetAccept( &netsrv1, &netsrv, NULL, COSM_NET_ACCEPT_NOWAIT )
    != COSM_PASS )
  {
    return -12;
  }

  /* Lets test pushing a buffer through. */
  if ( CosmNetSend( &netclient1, &bytes, buf1, 10 ) != COSM_PASS )
  {
    return -13;
  }

  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netsrv1, 15, wait_time )
    != COSM_PASS ) || ( bytes != 10 ) )
  {
    return -14;
  }

  for ( i = 0 ; i < 10 ; i++ )
  {
    if ( buf1[i] != buf2[i] )
    {
      return -15;
    }
  }

  /* Push some data 'backwards'. Change contents in buffer first.*/
  for ( i = 0 ; i < 10; i++ )
  {
      buf1[i] = (ascii) ( 10 - i );
  }

  if ( ( CosmNetSend( &netsrv1, &bytes, buf1, 10 ) != COSM_PASS )
    || ( bytes != 10 ) )
  {
    return -16;
  }

  /* and same again.. */
  if ( ( CosmNetSend( &netsrv1, &bytes, buf1, 10 ) != COSM_PASS )
    || ( bytes != 10 ) )
  {
    return -17;
  }

  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netclient1, 25, wait_time )
    != COSM_PASS ) || ( bytes != 20 ) )
  {
    return -18;
  }

  for ( i = 0; i < 10; i++ )
  {
    if ( ( buf2[i] != (ascii) ( 10 - i ) )
      || ( buf2[i + 10] != (ascii) ( 10 - i ) ) )
    {
      return -19;
    }
  }

  /*
    Lets try reading where there should be no data. buf3 should be
    zero initialized.
  */
  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netsrv1, 100, wait_time ) != COSM_PASS )
    || ( bytes != 0 ) )
  {
    /* Magically appearing data. No good. */
    return -20;
  }

  /* netsrv2's mode never gets set. Set it to TCP. */
  netsrv2.mode = COSM_NET_MODE_TCP;

  /* Nothing should be pending. */
  if ( CosmNetAccept( &netsrv2, &netsrv, NULL, COSM_NET_ACCEPT_NOWAIT )
    == COSM_PASS )
  {
    return -21;
  }

  /* Open up a second 'client' against the 'server'. */
  addr.type = COSM_NET_IPV4;
  addr.ip.v4 = my_addr.ip.v4;
  addr.port = netsrv.my_addr.port;
  if ( _COSM_NETOPEN( &netclient2, &addr ) != COSM_PASS )
  {
    return -22;
  }

  /* Accept the second connection. */
  if ( CosmNetAccept( &netsrv2, &netsrv, NULL, COSM_NET_ACCEPT_NOWAIT )
    == COSM_FAIL )
  {
    return -23;
  }

  /*
    Try sending some stuff on the new line. Make sure it doesn't
    echo to any other lines.
  */
  for ( i = 0 ; i < 15 ; i++ )
  {
    buf1[i] = (ascii) ( i * 2 );
  }

  if ( CosmNetSend( &netclient2, &bytes, buf1, 15 ) != COSM_PASS )
  {
    return -24;
  }

  /* Check the three sockets there should -not- be data on. */
  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netsrv1, 100, wait_time ) != COSM_PASS )
    || ( bytes != 0 ) )
  {
    return -25;
  }

  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netclient1, 100, wait_time )
    != COSM_PASS ) || ( bytes != 0 ) )
  {
    return -26;
  }

  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netclient2, 100, wait_time )
    != COSM_PASS ) || ( bytes != 0 ) )
  {
    return -27;
  }

  /* Now take in proper data. */
#if 0
  CosmYield();
  if ( ( CosmNetRecv( buf2, &bytes, &netsrv2, 100, wait_time )
    != COSM_PASS ) || ( bytes != 15 ) )
  {
    return -28;
  }

  for ( i = 0 ; i < 15 ; i++ )
  {
    if ( ( buf2[i] != buf1[i] ) || ( buf2[i] != (ascii) ( i * 2 ) ) )
    {
      return -29;
    }
  }
#endif

  /* Close, cleanup. */
  if ( CosmNetClose( &netsrv1 ) != COSM_PASS )
  {
    return -30;
  }

  if ( CosmNetClose( &netsrv2 ) != COSM_PASS )
  {
    return -31;
  }

  if ( CosmNetClose( &netclient1 ) != COSM_PASS )
  {
    return -32;
  }

#if ( ( OS_TYPE != OS_WIN32 ) && ( OS_TYPE != OS_WIN64 ) && \
  ( OS_TYPE != OS_FREEBSD ) && \
  ( ( OS_TYPE != OS_MACOSX ) || ( CPU_TYPE != CPU_X86 ) ) && \
  ( ( OS_TYPE != OS_MACOSX ) || ( CPU_TYPE != CPU_X64 ) ) )
  /* this one is tricky, it should fail on the second call, NOT coredump */
  if ( ( CosmNetSend( &netclient2, &bytes, buf1, 10 ) == COSM_PASS )
    && ( CosmNetSend( &netclient2, &bytes, buf1, 10 ) == COSM_PASS ) )
  {
    return -33;
    /* most likely Cosm_NetGlobalInit failure */
  }
#endif

/*
  if ( CosmNetClose( &netclient2 ) != COSM_PASS )
  {
    return -34;
  }
*/

  if ( CosmNetClose( &netsrv ) != COSM_PASS )
  {
    return -35;
  }

#ifdef __FIX_THIS
  if ( CosmNetRevDNS( (cosm_NET_HOSTNAME *) host_name, my_addr ) == COSM_FAIL )
  {
    return -36;
  }

  if ( CosmNetMyIP( &addr1, 1 ) == 0 )
  {
    return -38;
  }
#endif

  /* move this up higher */
  if ( ( CosmNetDNS( &my_addr, 1, "80.96.112.128" ) != 1 )
    || ( my_addr.ip.v4 != 0x50607080 )
    || ( my_addr.type != COSM_NET_IPV4 )
    || ( my_addr.port != 0 ) )
  {
    return -39;
  }

  /*
    Net ACL tests
  */

  return COSM_PASS;
#endif /* NO_NETWORKING */
}
