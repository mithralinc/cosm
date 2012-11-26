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

#include "cosm/os_math.h"
#include "cosm/time.h"
#include "cosm/os_file.h"
#include "cosm/os_net.h"

s32 CosmTime( cosmtime * dest, cosm_TIME_CORRECTION * corrections )
{
  cosmtime lt, diff, drift, result;

  /* Start with a few sanity checkes. */
  if ( ( dest == NULL ) || ( corrections == NULL ) )
  {
    return COSM_FAIL;
  }

  /* Get the lock on the correction. */
  CosmMutexInit( &corrections->lock );
  if ( CosmMutexLock( &corrections->lock, COSM_MUTEX_WAIT ) == COSM_FAIL )
  {
    return COSM_FAIL;
  }

  /* Check if initialized properly.*/
  if ( ( corrections->initialized == 0 ) )
  {
    CosmMutexUnlock( &corrections->lock );
    return COSM_FAIL;
  }

  if ( CosmSystemClock( &lt ) == COSM_FAIL )
  {
    CosmMutexUnlock( &corrections->lock );
    return COSM_FAIL;
  }

  if ( CosmS128Gt( corrections->time_last_set, lt ) )
  {
    CosmMutexUnlock( &corrections->lock );
    return COSM_FAIL;
  }

  /* time = clock + offset + drift * seconds( mytime - time_last_set ) */

  /* sneaky way to shift diff right 64 */
  diff = CosmS128S64( CosmS128Sub( lt, corrections->time_last_set ).hi );
  drift = CosmS128Mul( diff, corrections->time_drift );
  result = CosmS128Add( CosmS128Add( lt, drift ), corrections->time_offset );

  /* release the lock. */
  CosmMutexUnlock( &corrections->lock );

  *dest = result;

  return COSM_PASS;
}

s32 CosmTimeSet( cosm_TIME_CORRECTION * corrections,
  u32 * ip_list, u32 ip_count )
{
  /* INSERT CODE HERE */
  return -1;
}

s32 CosmTimeUnitsGregorian( cosm_TIME_UNITS * units, cosmtime time )
{
  s64 seconds, leaps;
  u32 max_days[] = { 30, 28, 30, 29, 30, 29, 30, 30, 29, 30, 29, 30 };
  u32 total_days[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273,
                       304, 334, 365 };

  if ( units == NULL )
  {
    return COSM_FAIL;
  }

  units->subsec = time.lo;
  seconds = time.hi;

  /* Day of week */
  units->wday = (u32) ( ( seconds / 86400LL ) %  7LL );

  /* January 1, 2000 is a Saturday */
  if ( seconds < 0 )
  {
    units->wday = ( units->wday + 5 ) % 7;
  }
  else
  {
    units->wday = ( units->wday + 6 ) % 7;
  }

  /* Year */
  /* 365.2425 * 24 * 60 * 60 = 0x1E18558 */
  /* First approximation of the year */
  units->year = seconds / (s64) 0x1E18558;
  seconds -= units->year * 365LL * 86400LL;

  /* Need a positive remaining number of seconds to simplify calculation */
  if ( seconds < 0 )
  {
    units->year--;
    seconds += 365LL * 86400LL;
  }

  /* Using the approximate number of year, also count for leap days */
  if ( units->year < 0 )
  {
    leaps = ( units->year / 4LL ) - ( units->year / 100LL )
      + ( units->year / 400LL );
  }
  else
  {
    leaps = ( units->year / 4LL ) - ( units->year / 100LL )
      + ( units->year / 400LL ) + 1LL;
  }
  if ( !( units->year < 0 ) && Cosm_TimeIsYearLeap( ( units->year + 2000LL ) ) )
  {
    leaps--;
  }

  seconds -= leaps * 86400LL;

  /* Set known year value and check if remainder is less than one year */
  if ( seconds < 0 )
  {
    units->year = units->year + 1999LL;
    if ( Cosm_TimeIsYearLeap( units->year ) )
    {
      seconds += 366LL * 86400LL;
    }
    else
    {
      seconds += 365LL * 86400LL;
    }
  }
  else
  {
    units->year = ( units->year + 2000LL );
  }

  /* If remainder is still more than one year, fix it */
  if ( !( seconds < 366LL * 86400LL )
    || ( !Cosm_TimeIsYearLeap( units->year )
    && !( seconds < 365LL * 86400LL ) ) )
  {
    if ( Cosm_TimeIsYearLeap( units->year ) )
    {
      seconds -= 366LL * 86400LL;
    }
    else
    {
      seconds -= 365LL * 86400LL;
    }
    units->year++;
  }

  /* Seconds */
  units->sec = (u32) ( seconds % 60LL );
  seconds /= 60LL;

  /* minutes */
  units->min = (u32) ( seconds % 60LL );
  seconds /= 60LL;

  /* Hours */
  units->hour = (u32) ( seconds % 24LL );
  seconds /= 24LL;

  /* Day of year */
  units->yday = (u32) seconds;

  /* Month */
  units->month = units->yday / 30;
  if ( units->month > 11 )
  {
    units->month = 11;
  }
  if ( Cosm_TimeIsYearLeap( units->year ) && units->yday >= 59 )
  {
    if ( total_days[units->month] + 1 > units->yday )
    {
      units->month--;
    }
    if ( total_days[units->month + 1] + 1 <= units->yday )
    {
      /* Can this happen on leap years? */
      units->month++;
    }
    if ( units->month > 1 )
    {
      /* Remove Feb 29 from seconds now */
      seconds--;
    }
  }
  else
  {
    if ( total_days[units->month] > units->yday )
    {
      units->month--;
    }
    if ( total_days[units->month + 1] <= units->yday )
    {
      /* Probably only useful on march 1st */
      units->month++;
    }
  }
  seconds -= (s64) total_days[units->month];
  if ( units->month > 11 )
  {
    /* Something is broken in the code */
    return COSM_FAIL;
  }

  /* Day of month */
  units->day = (u32) seconds;
  if ( units->day > max_days[units->month] )
  {
    /* Something is broken in the code */
    return COSM_FAIL;
  }

  return COSM_PASS;
}

s32 CosmTimeDigestGregorian( cosmtime * time,
  const cosm_TIME_UNITS * const units )
{
  s64 tmp, seconds;
  u32 max_days[] = { 30, 28, 30, 29, 30, 29, 30, 30, 29, 30, 29, 30 };
  s32 total_days[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273,
                       304, 334 };

  if ( ( time == NULL ) || ( units == NULL ) )
  {
    return COSM_FAIL;
  }

  /* years */
  tmp = ( units->year - 2000LL );
  seconds = ( tmp * 365LL ) + ( tmp / 4LL ) - ( tmp / 100LL )
    + ( tmp / 400LL );

  if ( ( units->year < 2000LL ) )
  {
    if ( ( units->month > 1 ) && Cosm_TimeIsYearLeap( units->year ) )
    {
      /* After Feb 29 on a leap year */
      seconds++;
    }
  }
  else
  {
    seconds++; /* Year 2000 leap day */
    if ( ( units->month < 2 ) && Cosm_TimeIsYearLeap( units->year ) )
    {
      /* Before Feb 29 on a leap year */
      seconds--;
    }
  }

  /* months */
  if ( units->month > 11 )
  {
    /* Invalid month number */
    return COSM_FAIL;
  }

  seconds += (s64) total_days[units->month];

  /* days */
  if ( units->day > max_days[units->month] )
  {
    /* Invalid number of days for that month */
    return COSM_FAIL;
  }

  if ( ( units->month == 1) && ( units->day == 28 ) )
  {
    /* Feb 29 -- Make sure the year IS leap */
    if ( !Cosm_TimeIsYearLeap( units->year ) )
    {
      return COSM_FAIL;
    }
  }

  seconds += (s64) units->day;

  /* Bound verifications */
  if ( !( units->year < 2000LL ) )
  {
    if ( seconds < ( units->year - 2000LL ) )
    {
      /* Overflow */
      return COSM_FAIL;
    }
  }
  else
  {
    if ( seconds > ( units->year - 2000LL ) )
    {
      /* Underflow */
      return COSM_FAIL;
    }
  }

  /* hours */
  if ( units->hour > 23 )
  {
    /* Invalid number of hours */
    return COSM_FAIL;
  }
  seconds *= 24LL;
  seconds += (s64) units->hour;

  /* minutes */
  if ( units->min > 59 )
  {
    /* Invalid number of minutes */
    return COSM_FAIL;
  }
  seconds *= 60LL;
  seconds += (s64) units->min;

  /* seconds */
  if ( units->sec > 59 )
  {
    /* !!! Accept more if it's on a leap second !!! */
    /* Invalid number of seconds */
    return COSM_FAIL;
  }
  seconds *= 60LL;
  seconds += (s64) units->sec;
  /* !!! add leap seconds here if necessary !!! */

  /* Now fill up the time struct */
  time->hi = seconds;
  if ( seconds < 0 )
  {
    time->lo = 0 - units->subsec;
  }
  else
  {
    time->lo = units->subsec;
  }

  return COSM_PASS;
}

/* low level */

s32 Cosm_TimeIsYearLeap( s64 year )
{
  /* Based on Great Britain Dominions (in sept. 1752) calendar change
     This includes USA(except Alaska), part of Canada, Australia,
     Ireland, Scotland and Wales */
  /* With no year 0, so before year 1, it was year -1 */

  if ( year > 1752LL )
  {
    /* Gregorian Calendar */
    /* There is no official year 4000 rule yet */
    if ( ( ( year % 4LL ) == 0 ) &&
      ( ( ( year % 100LL ) != 0 ) || ( ( year % 400LL ) == 0 ) ) )
    {
      /* Year is leap */
      return 1;
    }
    return 0;
  }
  if ( year > (s64) 7LL )
  {
    /* Corrected Julian Calendar */
    return ( ( year % 4LL ) == 0 );
  }
  if ( !( year < -45LL ) )
  {
    /* Wrong start with the Julian Calendar */
    if ( year > -9LL )
    {
      /* No leap years in that period to compensate for the extra days
         introduced in the previous years */
      return 0;
    }
    return ( ( year % 3LL ) == 0 );
  }
  /* Before the Julian Calendar, it was a real mess!!! */
  return 0;
}

/* testing */

s32 Cosm_TestTime( void )
{
  cosmtime time_test;

  if ( CosmSystemClock( &time_test ) != COSM_PASS )
  {
    return -1;
  }

  return COSM_PASS;
}
