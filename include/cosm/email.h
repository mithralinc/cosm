/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#ifndef COSM_EMAIL_H
#define COSM_EMAIL_H

#include "cosm/cputypes.h"
#include "cosm/os_net.h"

#define COSM_EMAIL_ERROR_TO       -1 /* to rejected */
#define COSM_EMAIL_ERROR_FROM     -2 /* from rejected */
#define COSM_EMAIL_ERROR_HOST     -3 /* Invalid SMTP host */
#define COSM_EMAIL_ERROR_MESSAGE  -4 /* message == NULL / length != 0 */
#define COSM_EMAIL_ERROR_NORELAY  -5 /* Relay not allowed */
#define COSM_EMAIL_ERROR_ABORTED  -6 /* Unable to complete transmission */

/* High Level Functions */

s32 CosmEmailSMTP( const cosm_NET_ADDR * smtp_server,
  const ascii * to, const ascii * from, const ascii * subject,
  const ascii * message, u32 length );
  /*
    Send SMTP email message of length length with subject using the given
    smtp_server host. The To: and From: fields will be set to to and from.
    SMTP is a 7-bit protocol so only plain text can be sent.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/* Low Level Functions */

s32 Cosm_EmailGetReply( ascii * reply, cosm_NET * net );
  /*
    Get the complete reply message and only return the numeric part, setting
    reply to a three character string. reply must be at least 4 characters
    long.
    Returns: COSM_PASS on success or COSM_FAIL on failure.
  */

/* testing */

s32 Cosm_TestEmail( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
