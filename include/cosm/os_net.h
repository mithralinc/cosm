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

#ifndef COSM_NET_H
#define COSM_NET_H

#include "cputypes.h"
#include "cosm/os_task.h"

#define COSM_NET_STATUS_CLOSED   0
#define COSM_NET_STATUS_OPENING  1
#define COSM_NET_STATUS_OPEN     2
#define COSM_NET_STATUS_LISTEN   3

#define COSM_NET_MODE_NONE  0
#define COSM_NET_MODE_TCP   85
#define COSM_NET_MODE_UDP   153

#define COSM_NET_ERROR_ADDRESS   -1  /* Unable to connect to host/port */
#define COSM_NET_ERROR_MYADDRESS -2  /* Unable to bind to my host/port */
#define COSM_NET_ERROR_FIREHOST  -3  /* Invalid firewall host */
#define COSM_NET_ERROR_FIREPORT  -4  /* Invalid firewall port */
#define COSM_NET_ERROR_FIREPASS  -5  /* Bad firewall password */
#define COSM_NET_ERROR_ORDER     -6  /* Connection not in the correct state */
#define COSM_NET_ERROR_CLOSED    -7  /* Connection closed */
#define COSM_NET_ERROR_TIMEOUT   -8  /* Wait time exceded */
#define COSM_NET_ERROR_MODE      -9  /* TCP/UDP mode conflict */
#define COSM_NET_ERROR_PARAM     -10 /* Parameter error */
#define COSM_NET_ERROR_FATAL     -11 /* Something very very bad */
#define COSM_NET_ERROR_SOCKET    -12 /* Internal socket error, now closed */
#define COSM_NET_ERROR_ADDRTYPE  -13 /* Bad addr type, no IPV6 support? */
#define COSM_NET_ERROR_NO_NET    -14 /* No networking support */

#define COSM_NET_ACCEPT_NOWAIT  0
#define COSM_NET_ACCEPT_WAIT    1

#define COSM_NET_PASS_LENGTH    64

#define COSM_NET_IPV4 4
#define COSM_NET_IPV6 6

PACKED_STRUCT_BEGIN
typedef struct cosm_NET_ADDR
{
  u32 type;
  u32 port;
  union
  {
    u32  v4;
    u128 v6;
  } ip;
} cosm_NET_ADDR;

typedef struct cosm_NET
{
  u64 handle;
  u32 status;
  u32 mode;
  cosm_NET_ADDR addr;
  cosm_NET_ADDR my_addr;
} cosm_NET;
PACKED_STRUCT_END

#define COSM_NET_MAX_HOSTNAME 1024

typedef ascii cosm_NET_HOSTNAME[COSM_NET_MAX_HOSTNAME];

#define COSM_NET_ALLOW  1
#define COSM_NET_DENY   0

typedef struct cosm_NET_ACL_ENTRY
{
  cosmtime expires;
  cosm_NET_ADDR addr;
  cosm_NET_ADDR mask;
  u32 permission;
  u32 null;  /* for memory alignment */
} cosm_NET_ACL_ENTRY;

typedef struct cosm_NET_ACL
{
  u32 length;
  u32 count;
  cosm_NET_ACL_ENTRY * list;
  cosm_MUTEX lock;
} cosm_NET_ACL;

/* Network send and receive functions */

s32 CosmNetOpen( cosm_NET * net, cosm_NET_ADDR * my_addr,
  const cosm_NET_ADDR * addr, u32 mode );
  /*
    Open a network connection to the remote address addr. my_addr is the
    address to connect from if possible and is set to the address actually
    connected from. NULL allows the OS to pick the best interface.
    If mode is COSM_NET_MODE_TCP, TCP protocol is used. If mode is
    COSM_NET_MODE_UDP, then the UDP protocol is used.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetOpenSocks( cosm_NET * net, const cosm_NET_ADDR * addr,
  const cosm_NET_ADDR * firehost, const ascii * firepass );
  /*
    CosmNetOpen for SOCKS firewalls, opening an IPv4 TCP connection to the
    host. firehost and firepass are the SOCKS firewall settings.
    Use semi-colon to separate username from password for socks5 firewalls.
    This functionality is depreciated.
    Returns: COSM_PASS on success, or an error code on failure.
  */

#define _COSM_NETOPEN( net, host ) \
  CosmNetOpen( net, NULL, host, COSM_NET_MODE_TCP )
  /*
    A macro for the common case of CosmNetOpen.
  */

s32 CosmNetSend( cosm_NET * net, u32 * bytes_sent, const void * data,
  u32 length );
  /*
    Send the data over the network connection. Returns once data is sent.
    bytes_sent is set to the number of bytes actually sent.
    Connection must be opened/accepted in COSM_NET_MODE_TCP mode.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetRecv( void * buffer, u32 * bytes_received, cosm_NET * net,
  u32 length, u32 wait_ms );
  /*
    Read whatever data is available, up to length bytes, into the buffer.
    If wait_ms is non-zero, then delay up to wait_ms milliseconds until
    length bytes have been received. bytes_read is set to the
    number of bytes actually read.
    Connection must be opened/accepted in COSM_NET_MODE_TCP mode.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetSendUDP( cosm_NET * net, const cosm_NET_ADDR * addr,
  const void * data, u32 length );
  /*
    Send length bytes of data to the addr, may or may not arrive.
    Connection must be opened in COSM_NET_MODE_UDP mode.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetRecvUDP( void * buffer, u32 * bytes_read, cosm_NET_ADDR * from,
  cosm_NET * net, u32 length, cosm_NET_ACL * acl, u32 wait_ms );
  /*
    Read a UDP packet into the buffer, if the buffer length is not long
    enough to hold the packet, the remaining data will be lost.
    If wait_ms is non-zero, then delay up to wait_ms milliseconds for
    a packet to arrive. Connection must be listening in
    COSM_NET_MODE_UDP mode. Any data from a host that fails the acl masks
    will be ignored.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetListen( cosm_NET * net, const cosm_NET_ADDR * addr, u32 mode,
  u32 queue );
  /*
    Set up a listening point on addr that will accept connections.
    Listen on addr if possible, if the IP is zero listen on all interfaces.
    If mode is COSM_NET_MODE_TCP, TCP protocol is used. If mode is
    COSM_NET_MODE_UDP, then the UDP protocol is used.
    If port is zero, let OS pick one. Allow queue connections to be
    queued before rejecting connections.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetAccept( cosm_NET * new_connection, cosm_NET * net,
  cosm_NET_ACL * acl, u32 wait );
  /*
    Accept a network connection if one is waiting, and fill in all the fields
    of new_connection. If wait is COSM_NET_ACCEPT_WAIT then do not return
    until a connection is accepted, otherwise if wait is
    COSM_NET_ACCEPT_NOWAIT return immediately. Any host that fails the
    acl masks will be ignored.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmNetClose( cosm_NET * net );
  /*
    Close the network connection and clear out any remaining data.
    Returns: COSM_PASS on success, or an error code on failure.
  */

u32 CosmNetDNS( cosm_NET_ADDR * addr, u32 count, ascii * name );
  /*
    Perform DNS lookup. Sets up to count addr's to the addresses of the
    named host.
    Returns: Number of IP's set, 0 indicates failure.
  */

s32 CosmNetRevDNS( cosm_NET_HOSTNAME * name, const cosm_NET_ADDR * addr );
  /*
    Perform reverse DNS. Sets name to the hostname of the ip.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

u32 CosmNetMyIP( cosm_NET_ADDR * addr, u32 count );
  /*
    Sets up to length addresses to the IP of the host. Expect at least 2 IPv4
    addresses and possibly 2+ IPv6 addresses as well. The order of returned
    addresses is meaningless.
    Returns: Number of addresses set, 0 indicates failure or no networking.
  */

s32 CosmNetACLAdd( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr,
  u32 mask_bits, u32 permission, cosmtime expires );
  /*
    Add the ip/mask pair to the acl. perm should be either COSM_NET_ALLOW or
    COSM_NET_DENY. expires is the time after which the entry will be deleted
    automatically, and should be based off of CosmSystemClock time.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmNetACLDelete( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr,
  u32 mask_bits );
  /*
    Delete the ip/mask entry from the acl.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmNetACLCheck( cosm_NET_ACL * acl, const cosm_NET_ADDR * addr );
  /*
    Test if an ip is accepted by the acl. An ip is accepted if it passed
    through each of the acl entries in order and has a COSM_NET_ALLOW setting
    at the end. The default is to allow i.e. a NULL acl.
    Returns: COSM_NET_ALLOW if accepted, or COSM_NET_DENY if rejected.
  */

void CosmNetACLFree( cosm_NET_ACL * acl );
  /*
    Free the internal acl data.
    Returns: nothing.
  */

s32 Cosm_NetGlobalInit( void );
  /*
    Initialize any global networking states that an OS needs.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 Cosm_NetClose( cosm_NET * net );
  /*
    Low level socket closing, sets the internal status. The socket will be
    closed when function returns.
    Returns: COSM_PASS on success, or an error code on failure.
  */

#if ( defined( NET_LOG_PACKETS ) )
Cosm_NetLogPacket( const cosm_NET_ADDR * addr, ascii * tag,
  u8 * data, u32 length );
#endif

/* testing */

s32 Cosm_TestOSNet( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
