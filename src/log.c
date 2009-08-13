/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2007 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "cosm/log.h"
#include "cosm/os_io.h"
#include "cosm/os_mem.h"

s32 CosmLogOpen( cosm_LOG * log, ascii * filename, u32 max_level, u32 mode )
{
  s32 result;

  if ( log == NULL )
  {
    return COSM_LOG_ERROR_INIT;
  }

  CosmMutexInit( &log->lock );
  if ( CosmMutexLock( &log->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_LOG_ERROR_INIT;
  }

  log->status = COSM_LOG_STATUS_NULL;

  if ( filename == NULL )
  {
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_NAME;
  }

  if ( ( mode != COSM_LOG_MODE_NUMBER ) && ( mode != COSM_LOG_MODE_BITS ) )
  {
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_MODE;
  }

  result = CosmMemSet( (void *) &log->file, sizeof( cosm_FILE ), 0 );
  if ( result != COSM_PASS )
  {
    /* Something bad has happened. */
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_ACCESS;
  }

  result = CosmFileOpen( &log->file, filename, COSM_FILE_MODE_CREATE |
    COSM_FILE_MODE_APPEND, COSM_FILE_LOCK_NONE );

  if ( result == COSM_FILE_ERROR_NAME )
  {
    /* Invalid filename */
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_NAME;
  }

  if ( result != COSM_PASS )
  {
    /* Problem creating or accessing the logfile. */
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_ACCESS;
  }

  /* We have an open file at this point.  Close it. */
  CosmFileClose( &log->file );

  /* Everything checks out.  Fill in the cosm_LOG structure and return. */
  CosmStrCopy( log->filename, filename, (u64) COSM_FILE_MAX_FILENAME );
  log->mode = mode;
  log->level = max_level;
  log->status = COSM_LOG_STATUS_INIT;

  CosmMutexUnlock( &log->lock );

  return COSM_PASS;
}

s32 CosmLog( cosm_LOG * log, u32 level, u32 echo, const utf8 * format, ... )
{
  va_list ap;
  s32 result;

  if ( ( log == NULL ) || ( format == NULL ) )
  {
    return COSM_LOG_ERROR_INIT;
  }

  if ( CosmMutexLock( &log->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_LOG_ERROR_INIT;
  }

  /* Sanity checks */
  if ( ( log->filename == NULL ) || ( log->status != COSM_LOG_STATUS_INIT ) ||
    ( ( log->mode != COSM_LOG_MODE_NUMBER ) &&
    ( log->mode != COSM_LOG_MODE_BITS ) ) )
  {
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_INIT;
  }

  /* Bail if we shouldn't write to the log. 0 = always write */
  if ( level != 0 )
  {
    if ( log->mode == COSM_LOG_MODE_NUMBER )
    {
      /* We're using 'number' */
      if ( level > log->level )
      {
        /* Then we do not log */
        CosmMutexUnlock( &log->lock );
        return COSM_PASS;
      }
    }
    else
    {
      /* We're using 'bits' */
      if ( ( level & log->level ) == 0 )
      {
        /* Then we do not log */
        CosmMutexUnlock( &log->lock );
        return COSM_PASS;
      }
    }
  }

  /* Looks like we're writing a log entry. */

  result = CosmMemSet( (void *) &log->file, sizeof( cosm_FILE ), 0 );
  if ( result != COSM_PASS )
  {
    /* Something bad has happened */
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_ACCESS;
  }

  result = CosmFileOpen( &log->file, log->filename, COSM_FILE_MODE_CREATE |
    COSM_FILE_MODE_APPEND, COSM_FILE_LOCK_WRITE );
  if ( result != COSM_PASS )
  {
    /* Problem creating or accessing the logfile. */
    CosmMutexUnlock( &log->lock );
    return COSM_LOG_ERROR_ACCESS;
  }

  /* Write to file */
  va_start( ap, format );
  Cosm_Print( &log->file, NULL, 0xFFFFFFFF, format, ap );
  va_end( ap );

  if ( echo == COSM_LOG_ECHO )
  {
    va_start( ap, format );
    Cosm_Print( NULL, NULL, 0xFFFFFFFF, format, ap );
    va_end( ap );
  }

  /* Ok. That's it. Close up and go home. */
  CosmFileClose( &log->file );
  CosmMutexUnlock( &log->lock );

  return COSM_PASS;
}

s32 CosmLogClose( cosm_LOG * log )
{
  if ( CosmMutexLock( &log->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_LOG_ERROR_INIT;
  }
  log->status = COSM_LOG_STATUS_NULL;
  CosmMutexUnlock( &log->lock );
  CosmMutexFree( &log->lock );

  return COSM_PASS;
}

s32 Cosm_TestLog( void )
{
  cosm_LOG log;
  cosm_FILE file;
  utf8 * correct, buf[512];
  u64 real_read;
  s32 ret;

  /* Tests 1-4 - Clear stuff */

  if ( CosmMemSet( &log, sizeof( cosm_LOG ), 0x00 ) != COSM_PASS )
  {
    return -1;
  }

  if ( CosmMemSet( &file, sizeof( cosm_FILE ), 0x00 ) != COSM_PASS )
  {
    return -2;
  }

  /* clean up from previous test */
  ret = CosmFileDelete( "test.log" );
  if ( ( ret != COSM_PASS ) && ( ret != COSM_FILE_ERROR_NOTFOUND ) )
  {
    return -4;
  }

  /*
    5. Test CosmLogOpen NULL log
    6. Test CosmLogOpen NULL filename
    7. Test CosmLogOpen invalid mode
    8. Test CosmLog and NULL log
    9. Test CosmLog and NULL format
  */

  if ( CosmLogOpen( (cosm_LOG *) NULL, "test.log", 0,
    COSM_LOG_MODE_NUMBER ) != COSM_LOG_ERROR_INIT )
  {
    return -5;
  }

  if ( CosmLogOpen( &log, NULL, 0, COSM_LOG_MODE_NUMBER )
    != COSM_LOG_ERROR_NAME )
  {
    return -6;
  }

  if ( CosmLogOpen( &log, "test.log", 0,
   ( COSM_LOG_MODE_NUMBER | COSM_LOG_MODE_BITS ) ) != COSM_LOG_ERROR_MODE )
  {
    return -7;
  }

  if ( CosmLog( (cosm_LOG *) NULL, 0, COSM_LOG_NOECHO, "foo" )
    != COSM_LOG_ERROR_INIT )
  {
    return -8;
  }

  if ( CosmLog( &log, 0, COSM_LOG_NOECHO, NULL ) != COSM_LOG_ERROR_INIT )
  {
    return -9;
  }

  /*
    Create a COSM_LOG_MODE_NUMBER log and
    use it for the first set of tests
    10. Initialise log
    11. Test log level 10
    12. Test log level 6
    13. Test log level 5
    14. Test log level 4 and %u
    15. Test log level 1 and %s
    16. Test log level 0
    17. Open logfile for reading
    18. Read logfile's contents
    19. Compare contents with expected
    20. Delete logfile
  */

  if ( CosmLogOpen( &log, "test.log", 5, COSM_LOG_MODE_NUMBER )
    != COSM_PASS )
  {
    return -10;
  }

  if ( CosmLog( &log, 10, COSM_LOG_NOECHO,
    "*** This shouldn't be logged*** \n" ) != COSM_PASS )
  {
    return -11;
  }

  if ( CosmLog( &log, 6, COSM_LOG_NOECHO,
    "*** This shouldn't be logged: %u ***\n", (u32) 123 )
    != COSM_PASS )
  {
    return -12;
  }

  if ( CosmLog( &log, 5, COSM_LOG_NOECHO,
    "This should be logged\n" ) != COSM_PASS )
  {
    return -13;
  }

  if ( CosmLog( &log, 4, COSM_LOG_NOECHO, "So should this: %u\n",
    (u32) 123 ) != COSM_PASS )
  {
    return -14;
  }

  if ( CosmLog( &log, 1, COSM_LOG_NOECHO, "%.6s <- That too\n",
    "foobar" ) != COSM_PASS )
  {
    return -15;
  }

  if ( CosmLog( &log, 0, COSM_LOG_NOECHO, "Yup, this too\n" )
    != COSM_PASS )
  {
    return -16;
  }

  CosmLogClose( &log );

  /* Check log */

  correct = "This should be logged\n\
So should this: 123\nfoobar <- That too\nYup, this too\n";

  if ( CosmFileOpen( &file, "test.log", COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    return -17;
  }

  CosmFileRead( buf, &real_read, &file, CosmStrBytes( correct ) );
  CosmFileClose( &file );
  if ( real_read != CosmStrBytes( correct ) )
  {
    return -18;
  }

  if ( CosmStrCmp( buf, correct, CosmStrBytes( correct ) ) != COSM_PASS )
  {
    return -19;
  }

  if ( CosmFileDelete( "test.log" ) != COSM_PASS )
  {
    return -20;
  }

  /*
    Create a COSM_LOG_MODE_BITS log and
    use it for the first set of tests
    21. Clear log struct
    22. Clear file struct
    23. Initialise log
    24. Test log level 0x00
    25. Test log level 0x01 & %X
    26. Test log level 0x04
    27. Test log level 0x10
    28. Test log level 0x40
    29. Test log level 0x22
    30. Test log level 0x82
    31. Open logfile for reading
    32. Read logfile's contents
    33. Compare contents with expected
    34. Delete logfile
  */

  if ( CosmMemSet( &log, sizeof( cosm_LOG ), 0x00 ) != COSM_PASS )
  {
    return -21;
  }

  if ( CosmMemSet( &file, sizeof( cosm_FILE ), 0x00 ) != COSM_PASS )
  {
    return -22;
  }

  if ( CosmLogOpen( &log, "test.log", 0x55, COSM_LOG_MODE_BITS )
    != COSM_PASS )
  {
    return -23;
  }

  if ( CosmLog( &log, 0x00, COSM_LOG_NOECHO, "This should be logged\n" )
    != COSM_PASS )
  {
    return -24;
  }

  if ( CosmLog( &log, 0x01, COSM_LOG_NOECHO,
    "This should also be logged: %X\n", (u32) 54321 ) != COSM_PASS )
  {
    return -25;
  }

  if ( CosmLog( &log, 0x04, COSM_LOG_NOECHO, "This should be logged\n" )
    != COSM_PASS )
  {
    return -26;
  }

  if ( CosmLog( &log, 0x10, COSM_LOG_NOECHO,
    "This should be logged too.\n" ) != COSM_PASS )
  {
    return -27;
  }

  if ( CosmLog( &log, 0x40, COSM_LOG_NOECHO,
    "This should be the last line.\n" ) != COSM_PASS )
  {
    return -28;
  }

  if ( CosmLog( &log, 0x22, COSM_LOG_NOECHO,
    "*** This shouldn't be logged ***\n" ) != COSM_PASS )
  {
    return -29;
  }

  if ( CosmLog( &log, 0x82, COSM_LOG_NOECHO,
    "*** This shouldn't be logged ***\n" ) != COSM_PASS )
  {
    return -30;
  }

  CosmLogClose( &log );

  /* Check log */

  correct = "This should be logged\n\
This should also be logged: D431\nThis should be logged\n\
This should be logged too.\nThis should be the last line.\n";

  if ( CosmFileOpen( &file, "test.log", COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    return -31;
  }

  CosmFileRead( buf, &real_read, &file, CosmStrBytes( correct ) );
  CosmFileClose( &file );
  if ( real_read != CosmStrBytes( correct ) )
  {
    return -32;
  }

  if ( CosmStrCmp( buf, correct, CosmStrBytes( correct ) ) != COSM_PASS )
  {
    return -33;
  }

  if ( CosmFileDelete( "test.log" ) != COSM_PASS )
  {
    return -34;
  }

  return COSM_PASS;
}
