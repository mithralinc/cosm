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

#include "cosm/cosm.h"

cosm_TEST __cosm_test_modules[COSM_TEST_MODULE_MAX + 2] =
{
  /* 0 */
  { NULL, NULL },

  /* CPU/OS  1-6 */
  { "os_math", Cosm_TestOSMath },
  { "os_task", Cosm_TestOSTask },
  { "os_mem", Cosm_TestOSMem },
  { "os_file", Cosm_TestOSFile },
  { "os_io", Cosm_TestOSIO },
  { "os_net", Cosm_TestOSNet },

  /* Utility  7-17 */
  { "bignum", Cosm_TestBigNum },
  { "buffer", Cosm_TestBuffer },
  { "config", Cosm_TestConfig },
  { "email", Cosm_TestEmail },
  { "hashtable", Cosm_TestHashTable },
  { "http", Cosm_TestHTTP },
  { "language", Cosm_TestLanguage },
  { "log", Cosm_TestLog },
  { "security", Cosm_TestSecurity },
  { "time", Cosm_TestTime },
  { "transform", Cosm_TestTransform },

  /* 18 */
  { NULL, NULL }
};

s32 CosmTest( s32 * failed_module, s32 * failed_test, s32 module_num )
{
  /* Run all the module test functions */
  s32 error;
  s32 i;

  error = 0;
  *failed_module = 0;
  *failed_test = 0;

  if ( module_num == 0 )
  {
    for ( i = 1 ; i <= COSM_TEST_MODULE_MAX ; i++ )
    {
      if ( ( error = (__cosm_test_modules[i].function)() ) != COSM_PASS )
      {
        *failed_module = -i;
        *failed_test = error;
        return -i;
      }
    }
  }
  else if ( module_num <= COSM_TEST_MODULE_MAX )
  {
    if ( ( error = (__cosm_test_modules[module_num].function)() ) != COSM_PASS )
    {
      *failed_module = -module_num;
      *failed_test = error;
      return -module_num;
    }
  }

  return COSM_PASS;
}
