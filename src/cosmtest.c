/*
  Copyright 1995-2019 Mithral Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
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

  if ( ( __cosm_test_modules[0].name != NULL )
    || ( __cosm_test_modules[0].function != NULL )
    || ( __cosm_test_modules[COSM_TEST_MODULE_MAX + 1].name != NULL )
    || ( __cosm_test_modules[COSM_TEST_MODULE_MAX + 1].function != NULL ) )
  {
    CosmPrint( "\nCosmTest module list wrong in cosmtest.c\n" );
    CosmProcessEnd( -1 );
  }

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
