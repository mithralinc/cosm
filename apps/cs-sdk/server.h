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

#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"

/* filenames the server will use */
#define SERVER_DATAFILE "server.dat"
#define SERVER_CONFIG   "server.cfg"
#define WORK_LOG        "work.log"
#define ACTIVITY_LOG    "activity.log"

void CatchSignal( int arg );
  /*
    Sets __shutdown_flag to 1 on signal. In the example it shuts down
    immediately, but with multi-connection or multi-thread versions
    you may want to delay shutdown.
    Returns: nothing.
  */

void Thread( void * arg );
  /*
    A worker thread, handles one connection.
    Returns: nothing.
  */

s32 Handler( cosm_HTTPD_REQUEST * request );
  /*
    Handlers HTTP requests
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 Initialize( cosm_NET * net );
  /*
    This function is responsible for loading in the unassigned work, assigned
    work, and other state information needed for the server. It also sets
    up the network listening port, and any other initial setup.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 Shutdown( void );
  /*
    This function closes any network connections, and saves all the state data
    such as unassigned work.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void Assign( cosm_NET * net, cosm_HTTPD_REQUEST * http_req, u32 use_http );
  /*
    This function takes the open network connection net and reads the
    remainder of a PACKET_REQUEST packet, assigns the client some work, tracks
    the assigned work, and then sends a PACKET_ASSIGNMENT packet to the client
    with the assigned work.
    Returns: nothing.
  */

void Accept( cosm_NET * net, cosm_HTTPD_REQUEST * http_req, u32 use_http );
  /*
    This function takes the open network connection net and reads the
    remainder of a PACKET_RESULTS packet, saves the completed work, logs it,
    and then returns a PACKET_ACCEPT packet to the client for acknowledgement.
    Returns: nothing.
  */

int main( int argc, char *argv[] );
  /*
    You know what this is for ;)
  */

#endif
