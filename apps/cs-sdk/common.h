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

#ifndef _COMMON_H
#define _COMMON_H

#include "cosm/cosm.h"

/* packet types, always the first u32 in the packet */
#define PACKET_TYPE_REQUEST     1
#define PACKET_TYPE_ASSIGNMENT  2
#define PACKET_TYPE_RESULTS     3
#define PACKET_TYPE_ACTIVE      4
#define PACKET_TYPE_ACCEPT      5

/* states for PACKET_ACTIVE */
#define STATE_INVALID         0
#define STATE_ALIVE           1
#define STATE_DONE            2

/* network settings */
#define NETPACKET_SERVER     "127.0.0.1" /* server hostname */
#define NETPACKET_PORT       "10204"     /* server port */
#define NETPACKET_HTTPSERVER "127.0.0.1" /* http host */
#define NETPACKET_HTTPPORT   "8081"      /* http port */

/* client asking for a work assignment */
typedef struct
{
  u32 type;
  u32 pad; /* all data must align on 8 byte boundaries */
  u64 speed;
} PACKET_REQUEST;

/* assignment given to client by server */
typedef struct
{
  u32 type;
  /* project assignment data */
  u32 start;
  u32 end;
} PACKET_ASSIGNMENT;

/* results sent to server */
typedef struct
{
  u32 type;
  /* project results */
  u32 start;
  u64 total;
  u32 end;
  u32 cpu;
  u32 os;
  ascii email[32];
} PACKET_RESULTS;

/* ACK/NACK after server accepts results */
typedef struct
{
  u32 type;
  u32 result;
} PACKET_ACCEPT;

/* live work entity, in progress on the client side */
typedef struct
{
  u32 type;
  /* project temporary data */
  u32 start;
  u32 current;
  u32 end;
  u64 total;
  u32 state;
} PACKET_ACTIVE;

#endif
