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

#ifndef _CLIENT_H
#define _CLIENT_H

#include "common.h"

/* filenames the client will use */
#define CLIENT_DATAFILE  "client.dat"
#define CLIENT_CONFIG    "client.cfg"
#define CLIENT_LOG       "client.log"

void catch_signal( int arg );
  /*
    Sets __shutdown_flag to 1 on signal.
    Returns: nothing.
  */

u32 Load( void );
  /*
    Load in all the PACKET_ACTIVE's from SUSPEND_FILENAME.
    Usually only done at startup.
    Returns: Number of PACKET_ACTIVE structures loaded.
  */

s32 Save( void );
  /*
    Save all the valid (active, done) PACKET_ACTIVE's to SUSPEND_FILENAME.
    This is done both at shutdown and for checkpointing.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

u32 GetWork( PACKET_ACTIVE * work, u32 count );
  /*
    Connect to the server, request a days worth of work with a PACKET_REQUEST
    packet, and read back the PACKET_ASSIGNMENT packet, storing
    the assigned work locally. If a given work is already loaded, it will
    not overwrite it.
    Returns: Number of PACKET_ACTIVE's received.
  */

u32 PutWork( PACKET_ACTIVE * work, u32 count );
  /*
    Connect to the server, send in a PACKET_RESULTS packet, and expect back a
    PACKET_ACCEPT packet. Once the work is acknowledged, it can be deleted.
    Will only send work that is done.
    Returns: Number of PACKET_ACTIVE's sent in.
  */

s32 Connect( cosm_NET * net, cosm_HTTP * http );
  /*
    This is a helper function used by GetWork and PutWork due to the
    complexity of connecting possibly with a firewall. net or http will be
    connected to the server and in a connected state upon return.
    Returns: COSM_PASS on successful connect to the server, or COSM_FAIL on
      failure.
  */

s32 Run( PACKET_ACTIVE * work, u64 iterations );
  /*
    Process the work for up to iterations loops. The algorithms used
    will depend entirely on the project. Before returning (without error)
    from this function the work must be in a checkpointable state.
    Returns: STATE_INVALID on error, STATE_DONE if the work was finished, or
      STATE_ALIVE if there is more work to be done.
  */

u64 Speed( void );
  /*
    Benchmark the client speed. This will be used both for checkpointing and
    to request a days worth of work.
    Returns: Number of iterations per second (or the timespan set) that the
      client can handle, or a default speed on error.
  */

s32 Test( void );
  /*
    Run selftests for the client, including CosmTest().
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

s32 Config( void );
  /*
    Asks the user for all needed configuration information. This will be
    logging level, network settings, and other project specific data.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

int main( int argc, char * argv[] );
  /*
    You know what this is for ;)
  */

#endif
