/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Cosm Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#ifndef COSM_TIME_H
#define COSM_TIME_H

#include "cosm/cputypes.h"
#include "cosm/os_task.h"

/*
  Times are signed s128's, 64b.64b format based 0 = 00:00:00 UTC, Jan 1,
  2000 AD. This gives a Range of +/- 2.923E11 years and a resolution of
  5.421E-20 seconds. This allows times from bang to crunch - assuming a closed
  universe (if it's not closed, we have bigger problems). It's also useful for
  timing the half lives of particles like Neutral Sigma Baryons.

  Useful constants:
  Beginning of time: 80000000, 00000001
  End of time: 7FFFFFFF, FFFFFFFF
  microsecond: 000010C6, F7A0B5ED
  millisecond: 00418937, 4BC6A7EF
  centisecond: 028F5C28, F5C28F5C
  POSIX epoc = Cosm + 946684800 (0x386D4380)
*/

#define COSM_TIME_CENTISECOND 0x028F5C28F5C28F5CLL
#define COSM_TIME_MILLISECOND 0x004189374BC6A7EFLL
#define COSM_TIME_MICROSECOND 0x000010C6F7A0B5EDLL
#define COSM_TIME_POSIX_DELTA 946684800

/*
  Use:
  const utf8 months[12] = COSM_TIME_MONTHS;
  const utf8 days[7] = COSM_TIME_DAYS;
  etc...
*/
#define COSM_TIME_MONTHS { "January", "February", \
  "March", "April", "May", \
  "June", "July", "August", \
  "September", "October", "November", \
  "December" }

#define COSM_TIME_MONTHS3 { "Jan", "Feb", \
  "Mar", "Apr", "May", "Jun", \
  "Jul", "Aug", "Sep", "Oct", \
  "Nov", "Dec" }

#define COSM_TIME_DAYS { "Sunday", "Monday", \
  "Tuesday", "Wednesday", "Thursday", \
  "Friday", "Saturday" }

#define COSM_TIME_DAYS3 { "Sun", "Mon", \
    "Tue", "Wed", "Thu", "Fri", \
    "Sat" }

typedef struct cosm_TIME_UNITS
{
  /* all values are zero based */
  s64 year;   /* year */
  u32 month;  /* month */
  u32 day;    /* day of month */
  u32 wday;   /* day of week */
  u32 yday;   /* day of year */
  u32 hour;   /* hours */
  u32 min;    /* minutes */
  u32 sec;    /* seconds */
  u64 subsec; /* sub-seconds, note: unsigned unlike cosmtime */
} cosm_TIME_UNITS;

typedef struct cosm_TIME_CORRECTION
{
  cosmtime time_last_set;  /* when originally set, for setting drift */
  cosmtime time_offset;    /* offset from system time */
  cosmtime time_drift;     /* drift/sec */
  u64     initialized;    /* setup or not */
  cosm_MUTEX lock;
} cosm_TIME_CORRECTION;

s32 CosmTime( cosmtime * dest, cosm_TIME_CORRECTION * corrections );
  /*
    Get the current time. Already Synchronized with network time.
    Compensate for system clock drift and correct accordingly.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmTimeSet( cosm_TIME_CORRECTION * corrections,
  u32 * ip_list, u32 ip_count );
  /*
    Synchronize with network time. Dont attempt to set the system clock even
    though that would be nice. Set the value of time_offset and related values
    relative to the system clock instead. Use the ip_list as the list of
    time servers to sync with.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmTimeUnitsGregorian( cosm_TIME_UNITS * units, cosmtime time );
  /*
    Fill in the cosm_TIME_UNITS structure with the time in Gregorian format.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmTimeDigestGregorian( cosmtime * time,
  const cosm_TIME_UNITS * const units );
  /*
    Convert back to a cosmtime value from Gregorian, sanity check the values.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/*
  With the above we can print whatever format we want.
  However, the year/month/day order should always be: year, month, day
  Since this is no fun to reverse parse, make sure we never need to.
  Keep all data in machine format, and only display it when needed.
*/

/* low level */

s32 Cosm_TimeIsYearLeap( s64 year );
  /*
    Check if year is a leap year.
    Returns: 1 if the year is a leap year, or 0 if it is not a leap year.
  */

/* testing */

s32 Cosm_TestTime( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
