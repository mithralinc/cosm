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

#ifndef COSM_LOG_H
#define COSM_LOG_H

#include "cosm/cputypes.h"
#include "cosm/os_file.h"
#include "cosm/os_task.h"

#define COSM_LOG_STATUS_NULL  0
#define COSM_LOG_STATUS_INIT  1

#define COSM_LOG_MODE_NUMBER  1
#define COSM_LOG_MODE_BITS    2

#define COSM_LOG_ECHO    3
#define COSM_LOG_NOECHO  6

#define COSM_LOG_ERROR_MODE    -1  /* Invalid mode */
#define COSM_LOG_ERROR_NAME    -2  /* Invalid filename */
#define COSM_LOG_ERROR_INIT    -3  /* Log not initialized */
#define COSM_LOG_ERROR_ACCESS  -4  /* File access error */

typedef struct cosm_LOG
{
  cosm_FILENAME filename;
  cosm_FILE file;
  u32 status;
  u32 mode;
  u32 level;
  cosm_MUTEX lock;
} cosm_LOG;

s32 CosmLogOpen( cosm_LOG * log, ascii * filename, u32 max_level, u32 mode );
  /*
    Initialize the log, setting the filename, max_level, and mode.
    If COSM_LOG_MODE_NUMBER then log any message with an equal or lower level
    than max_. If COSM_LOG_MODE_BITS then log any message with it's level
    matching a bit in max_level. Initializing a log with a NULL filename or
    invalid mode causes failure.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmLog( cosm_LOG * log, u32 level, u32 echo, const utf8 * format, ... );
  /*
    Write the formatted string to the log. See CosmPrint for format info.
    The string will be logged only if the level is acceptable to the open
    log. Level 0 will always logged. If echo is COSM_LOG_ECHO and the level
    passes then the string will also be printed to the standard output device.
    Otherwise if level does not pass or echo is COSM_LOG_NOECHO nothing will
    be printed.
    Returns: COSM_PASS on success, or an error code on failure
  */

s32 CosmLogClose( cosm_LOG * log );
  /*
    Close the file.
    Returns: COSM_PASS on success.
  */

/* testing */

s32 Cosm_TestLog( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
