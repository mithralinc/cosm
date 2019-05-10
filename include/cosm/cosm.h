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

/**
\file cosm.h
\brief Cosm master include file.
*/

#ifndef COSM_H
#define COSM_H

/* include everything users need */

#include "cosm/cputypes.h"

/* CPU/OS layer */
#include "cosm/os_file.h"
#include "cosm/os_io.h"
#include "cosm/os_math.h"
#include "cosm/os_mem.h"
#include "cosm/os_net.h"
#include "cosm/os_task.h"

/* Utility layer */
#include "cosm/bignum.h"
#include "cosm/buffer.h"
#include "cosm/config.h"
#include "cosm/email.h"
#include "cosm/hashtable.h"
#include "cosm/http.h"
#include "cosm/language.h"
#include "cosm/log.h"
#include "cosm/security.h"
#include "cosm/time.h"
#include "cosm/transform.h"

/* testing */

/**
\defgroup TEST Unit Test Functions
Run the Cosm unit tests.
\addtogroup TEST
@{
*/

/**
Structure for self test name and function.
*/
typedef struct cosm_TEST
{
  const char * name;     /**< Name of test file. */
  s32 (*function)(void); /**< Self test function name. */
} cosm_TEST;

#define COSM_TEST_MODULE_MAX 17
extern cosm_TEST __cosm_test_modules[COSM_TEST_MODULE_MAX + 2];

/**
Run all Cosm self tests. The function's code is in cosmtest.c

\param[out] failed_module Set to the failed module.
\param[out] failed_test Set to the failed test.
\param[in] module_num If module_num is 0, then test all Cosm functions
  otherwise test only the module corresponding to module_num.
\return COSM_PASS on success, or a negative number corresponding to the
  module that failed.
\code
  s32 module;
  s32 test;

  CosmPrint( "Running system tests... " );
  if ( CosmTest( &module, &test, 0 ) != COSM_PASS )
  {
    CosmPrint( "Test failure in module %.16s %i.\n",
      __cosm_test_modules[( -module > COSM_TEST_MODULE_MAX ) ?
      0 : -module].name, test );
    CosmProcessEnd( module );
  }
  CosmPrint( "all passed.\n" );
\endcode
*/
s32 CosmTest( s32 * failed_module, s32 * failed_test, s32 module_num );

/**
@}
*/

/** @cond SKIPED */
#if ( !defined( ALLOW_UNSAFE_C ) )
/*
  We define macros to catch any unsafe (dangerous) functions used.
  Any working compiler should spit out warnings.
*/
#define gets( a ) unsafe_gets_use_CosmInputA()
#define memcopy( a, b, c ) unsafe_memcpy_use_CosmMemCopy()
#define strcpy( a, b ) unsafe_strcpy_use_CosmStrCopy()
#define strncpy( a, b, c ) unsafe_strncpy_use_CosmStrCopy()
/*
  We need to define macros to catch any deprecated and non-portable
  function used. Any working compiler should spit out warnings.
*/
#define calloc( a, b ) replace_calloc_use_CosmMemAlloc()
#define malloc( a ) replace_malloc_use_CosmMemAlloc()
#define realloc( a, b ) replace_realloc_use_CosmMemRealloc()
#define free( a, b ) replace_free_use_CosmMemFree()
#define abort( ) replace_abort_use_CosmProcessEnd()
#define exit( a ) replace_exit_use_CosmProcessEnd()
#define rand( a ) replace_rand_use_CosmPRNG()
#define srand( a ) replace_srand_use_CosmPRNG()
#define fflush( a ) replace_fflush_use_CosmFileOpen()
#define fopen( a, b ) replace_fopen_use_CosmFileOpen()
#define freopen( a, b, c ) replace_freopen_use_CosmFileOpen()
#define printf( a ) replace_printf_use_CosmPrint()
#define sprintf( a ) replace_sprintf_use_CosmPrintStr()
#define snprintf( a ) replace_snprintf_use_CosmPrintStr()
#define fprintf( a ) replace_fprintf_use_CosmPrintFile()
#define vprintf( a ) replace_vprintf_use_Cosm_Print()
#define vfprintf( a ) replace_vfprintf_use_Cosm_Print()
#define vsprintf( a ) replace_vsprintf_use_Cosm_Print()
#define vsnprintf( a ) replace_vsnprintf_use_Cosm_Print()
#define raise( a ) replace_raise_use_CosmSignal()
#endif /* ALLOW_UNSAFE_C */
/** @endcond */

#endif
