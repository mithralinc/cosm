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

#ifndef COSM_CONFIG_H
#define COSM_CONFIG_H

#include "cosm/cputypes.h"
#include "cosm/os_task.h"

/*
  Dealing with config files

<begin file>
[section1]\n
key1=value1\n
key2=value2\n
\n
[section2]\n
key1=value1\n
key2=value2\n
\n
</end file>

The Rules:
  These are binary files, not text. You're not supposed to have to edit them.
  No linefeeds (\n) in section/keys/values - those are line separators.
  No '=' or '[' in keys, and avoid ']'.
  Everything is case sensitive.
  Blank lines are ignored, but added at the end of sections for humans.
  Any improperly formatted line is discarded.
  Comment lines don't exist, that's what documentation is for.
  The majority of cases the config will only be read, so optimize for
  reading.
*/

#define COSM_CONFIG_DELETED   0  /* Key deleted */
#define COSM_CONFIG_NORMAL    1  /* Part of main memory block */
#define COSM_CONFIG_ALLOCATED 2  /* Allocated outside the main block */

typedef struct cosm_CONFIG_KEY
{
  /* Key/value pairs are done with one alloc/free for both when needed */
  u32 length; /* length of space allocated for key+value and 2 \0 */
  u32 flag;
  utf8 * key;
  utf8 * value;
} cosm_CONFIG_KEY;

typedef struct cosm_CONFIG_SECTION
{
  u32 length; /* length of space for section name and \0 */
  u32 flag;
  utf8 * section;
  cosm_CONFIG_KEY * keys;
  u32 key_count;
} cosm_CONFIG_SECTION;

typedef struct cosm_CONFIG
{
  utf8 * memory;
  cosm_CONFIG_SECTION * sections;
  u32 section_count;
  cosm_MUTEX lock;
} cosm_CONFIG;

/* Config Functions */

s32 CosmConfigLoad( cosm_CONFIG * config, const ascii * filename );
  /*
    Attempt to open the filename and read in the config data.
    If filename is NULL, create an empty config.
    Returns: COSM_PASS on success, or a COSM_FILE_ERROR_* on failure.
  */

s32 CosmConfigSave( const cosm_CONFIG * config, const ascii * filename );
  /*
    Write out the config data to the file.
    Returns: COSM_PASS on success, or a COSM_FILE_ERROR_* on failure.
  */

s32 CosmConfigSet( cosm_CONFIG * config, const utf8 * section,
  const utf8 * key, const utf8 * value );
  /*
    Set the value for the section/key pair. If value is NULL then key is
    deleted.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

const utf8 * CosmConfigGet( cosm_CONFIG * config, const utf8 * section,
  const utf8 * key );
  /*
    Get the value of the section/key pair. The returned pointer will
    no longer be valid once another CosmConfig* call is made.
    Returns: A pointer to the value on success, or NULL on failure.
  */

void CosmConfigFree( cosm_CONFIG * config );
  /*
    Free the internal config data.
    Returns: nothing.
  */

/* testing */

s32 Cosm_TestConfig( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
