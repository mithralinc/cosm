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

#include "cosm/config.h"
#include "cosm/os_io.h"
#include "cosm/os_file.h"
#include "cosm/os_mem.h"

s32 CosmConfigLoad( cosm_CONFIG * config, const ascii * filename )
{
  cosm_FILE file;
  cosm_CONFIG_SECTION * tmp_section;
  u64 length, bytes_read;
  utf8 * ptr, * eol, * section, * key, * value;
  s32 error;
  u32 strlen;
  u32 i, j;

  if ( config == NULL )
  {
    return COSM_FILE_ERROR_DENIED;
  }

  CosmMutexInit( &config->lock );
  if ( CosmMutexLock( &config->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_FILE_ERROR_DENIED;
  }

  if ( filename == NULL )
  {
    if ( config->section_count > 0 )
    {
      /* already in use */
      CosmMutexUnlock( &config->lock );
      return COSM_FILE_ERROR_DENIED;
    }
    /* fill in struct */
    config->memory = NULL;
    config->sections = NULL;
    config->section_count = 0;
  }
  else
  {
    CosmMemSet( &file, sizeof( cosm_FILE ), 0 );

    if ( ( error = CosmFileOpen( &file, filename, COSM_FILE_MODE_READ,
      COSM_FILE_LOCK_READ ) ) != COSM_PASS )
    {
      CosmMutexUnlock( &config->lock );
      return error;
    }
    if ( ( error = CosmFileLength( &length, &file ) ) != COSM_PASS )
    {
      CosmFileClose( &file );
      CosmMutexUnlock( &config->lock );
      return error;
    }
    config->sections = NULL;
    config->section_count = 0;
    if ( ( config->memory = CosmMemAlloc( length ) )
      == NULL )
    {
      CosmFileClose( &file );
      CosmMutexUnlock( &config->lock );
      return COSM_FILE_ERROR_DENIED;
    }

    CosmFileRead( config->memory, &bytes_read, &file, length );
    if ( bytes_read != length )
    {
      CosmFileClose( &file );
      CosmMemFree( config->memory );
      CosmMutexUnlock( &config->lock );
      return COSM_FILE_ERROR_DENIED;
    }
    CosmFileClose( &file );

    /* parse */
    ptr = config->memory;
    while ( ( section = CosmStrChar( ptr, '[', (u32) length ) ) != NULL )
    {
      section++;
      ptr = CosmStrChar( ptr, ']', (u32) length );
      *ptr = 0;
      ptr++;
      ptr++;

      if ( config->section_count == 0 )
      {
        if ( ( config->sections = (cosm_CONFIG_SECTION *)
          CosmMemAlloc( sizeof( cosm_CONFIG_SECTION ) ) ) == NULL )
        {
          CosmMutexUnlock( &config->lock );
          return COSM_FILE_ERROR_DENIED;
        }
        config->section_count = 1;
      }
      else
      {
        if ( ( config->sections = (cosm_CONFIG_SECTION *)
          CosmMemRealloc( config->sections, (u64) ( config->section_count + 1 )
            * sizeof( cosm_CONFIG_SECTION ) ) )
          == NULL )
        {
          CosmMutexUnlock( &config->lock );
          return COSM_FILE_ERROR_DENIED;
        }
        CosmMemSet( &config->sections[config->section_count],
          sizeof( cosm_CONFIG_SECTION ), 0 );
        config->section_count += 1;
      }
      i = config->section_count - 1;
      config->sections[i].flag = COSM_CONFIG_NORMAL;
      config->sections[i].length = CosmStrBytes( section ) + 1;
      config->sections[i].section = section;
      config->sections[i].key_count = 0;
      config->sections[i].keys = NULL;

      /* now add the keys */
      tmp_section = &config->sections[i];
      while ( *ptr != '[' )
      {
        if ( ( eol = CosmStrChar( ptr, '\n', (u32) length ) ) == NULL )
        {
          /* no more lines */
          break;
        }
        *eol = 0;
        eol++;
        if ( ( value = CosmStrChar( ptr, '=', (u32) length ) ) != NULL )
        {
          key = ptr;
          *value = 0;
          value++;
          ptr = eol;

          /* ignore empty lines */
          while ( *ptr == '\n' )
          {
            ptr++;
          }

          if ( tmp_section->key_count == 0 )
          {
            if ( ( tmp_section->keys = (cosm_CONFIG_KEY *)
              CosmMemAlloc( sizeof( cosm_CONFIG_KEY ) ) ) == NULL )
            {
              CosmMutexUnlock( &config->lock );
              return COSM_FILE_ERROR_DENIED;
            }
            tmp_section->key_count = 1;
          }
          else
          {
            if ( ( tmp_section->keys = (cosm_CONFIG_KEY *)
              CosmMemRealloc( tmp_section->keys, (u64)
                ( tmp_section->key_count + 1 ) * sizeof( cosm_CONFIG_KEY ) ) )
              == NULL )
            {
              CosmMutexUnlock( &config->lock );
              return COSM_FILE_ERROR_DENIED;
            }
            CosmMemSet( &tmp_section->keys[tmp_section->key_count],
              sizeof( cosm_CONFIG_KEY ), 0 );
            tmp_section->key_count += 1;
          }

          j = tmp_section->key_count - 1;

          strlen = CosmStrBytes( key ) + CosmStrBytes( value );
          strlen += 2;

          tmp_section->keys[j].key = key;
          tmp_section->keys[j].value = value;
          tmp_section->keys[j].length = strlen;
          tmp_section->keys[j].flag = COSM_CONFIG_NORMAL;
        } /* if key/value pair */
      } /* while section */
    } /* while more to file */
  }

  CosmMutexUnlock( &config->lock );

  return COSM_PASS;
}

s32 CosmConfigSave( const cosm_CONFIG * config, const ascii * filename )
{
  u32 i, j, flag;
  cosm_CONFIG_SECTION * tmp_section;
  cosm_FILE file;
  s32 error;

  if ( ( config == NULL ) || ( filename == NULL ) )
  {
    return COSM_FILE_ERROR_DENIED;
  }

  CosmMemSet( &file, sizeof( cosm_FILE ), 0 );

  if ( ( error = CosmFileOpen( &file, filename, COSM_FILE_MODE_WRITE
    | COSM_FILE_MODE_CREATE | COSM_FILE_MODE_TRUNCATE, COSM_FILE_LOCK_WRITE ) )
    != COSM_PASS )
  {
    return error;
  }

  if ( CosmMutexLock( (cosm_MUTEX *) &config->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_FILE_ERROR_DENIED;
  }

  for ( i = 0 ; i < config->section_count; i++ )
  {
    if ( ( config->sections[i].flag == COSM_CONFIG_NORMAL )
      || ( config->sections[i].flag == COSM_CONFIG_ALLOCATED ) )
    {
      tmp_section = &config->sections[i];
      flag = 0;
      for ( j = 0 ; j < tmp_section->key_count; j++ )
      {
        if ( ( tmp_section->keys[j].flag == COSM_CONFIG_NORMAL )
          || ( tmp_section->keys[j].flag == COSM_CONFIG_ALLOCATED ) )
        {
          if ( flag == 0 )
          {
            /* this section contains active keys, write header */
            CosmPrintFile( &file, "[%.*s]\n",
              CosmStrBytes( tmp_section->section ),
              tmp_section->section );
            flag = 1;
          }
          /* write the key/value */
          CosmPrintFile( &file, "%.*s=%.*s\n",
            CosmStrBytes( tmp_section->keys[j].key ),
            tmp_section->keys[j].key,
            CosmStrBytes( tmp_section->keys[j].value ),
            tmp_section->keys[j].value );
        }
      }
      if ( flag == 1 )
      {
        CosmPrintFile( &file, "\n" );
      }
    }
  }

  CosmFileClose( &file );
  CosmMutexUnlock( (cosm_MUTEX *) &config->lock );

  return COSM_PASS;
}

s32 CosmConfigSet( cosm_CONFIG * config, const utf8 * section,
  const utf8 * key, const utf8 * value )
{
  u32 i, j;
  cosm_CONFIG_SECTION * tmp_section;
  u32 length;
  u32 key_len, val_len;

  if ( ( config == NULL ) || ( section == NULL ) || ( key == NULL ) )
  {
    return COSM_FAIL;
  }

  /* calculate length of key+value + 2 \0 */
  key_len = CosmStrBytes( key ) + 1;
  val_len = CosmStrBytes( value ) + 1;
  length = key_len + val_len;

  if ( CosmMutexLock( &config->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  for ( i = 0 ; i < config->section_count; i++ )
  {
    if ( CosmStrCmp( section, config->sections[i].section,
      CosmStrBytes( section ) ) == 0 )
    {
      /* found the section */
      tmp_section = &config->sections[i];
      for ( j = 0 ; j < tmp_section->key_count; j++ )
      {
        if ( CosmStrCmp( key, tmp_section->keys[j].key,
          CosmStrBytes( key ) ) == 0 )
        {
          /* found an existing key */
          if ( value == NULL )
          {
            /* mark deleted */
            if ( tmp_section->keys[j].flag == COSM_CONFIG_ALLOCATED )
            {
              tmp_section->keys[j].length = 0;
              CosmMemFree( tmp_section->keys[j].key );
              tmp_section->keys[j].key = NULL;
            }
            tmp_section->keys[j].flag = COSM_CONFIG_DELETED;
          }
          else if ( length < tmp_section->keys[j].length )
          {
            /* big enough, copy over */
            CosmStrCopy( tmp_section->keys[j].value, value, val_len );
            if ( tmp_section->keys[j].flag == COSM_CONFIG_DELETED )
            {
              tmp_section->keys[j].flag = COSM_CONFIG_NORMAL;
            }
          }
          else if ( tmp_section->keys[j].length > 0 )
          {
            /* expand */
            if ( ( tmp_section->keys[j].flag == COSM_CONFIG_NORMAL )
              || ( tmp_section->keys[j].flag == COSM_CONFIG_DELETED ) )
            {
              /* do allocate, copy key and value */
              if ( ( tmp_section->keys[j].key =
                 CosmMemAlloc( (u64) length ) ) == NULL )
              {
                CosmMutexUnlock( &config->lock );
                return COSM_FAIL;
              }
              CosmStrCopy( tmp_section->keys[j].key, key, key_len );
              tmp_section->keys[j].value =
                 CosmMemOffset( tmp_section->keys[j].key, key_len );
              CosmStrCopy( tmp_section->keys[j].value, value, val_len );
              tmp_section->keys[j].flag = COSM_CONFIG_ALLOCATED;
              tmp_section->keys[j].length = length;
            }
            else
            {
              /* realloc bigger, fix * value, then copy new value */
              if ( ( tmp_section->keys[j].key = CosmMemRealloc(
                tmp_section->keys[j].key, (u64) length ) ) == NULL )
              {
                CosmMutexUnlock( &config->lock );
                return COSM_FAIL;
              }
              tmp_section->keys[j].value = CosmMemOffset(
                tmp_section->keys[j].key, key_len );
              CosmStrCopy( tmp_section->keys[j].value, value, val_len );
            }
          }
          else
          {
            /* alloc'd then free'd, create new */
            if ( ( tmp_section->keys[j].key =
               CosmMemAlloc( (u64) length ) ) == NULL )
            {
              CosmMutexUnlock( &config->lock );
              return COSM_FAIL;
            }
            CosmStrCopy( tmp_section->keys[j].key, key, key_len );
            tmp_section->keys[j].value =
               CosmMemOffset( tmp_section->keys[j].key, key_len );
            CosmStrCopy( tmp_section->keys[j].value, value, val_len );
            tmp_section->keys[j].length = length;
            tmp_section->keys[j].flag = COSM_CONFIG_ALLOCATED;
          }
          CosmMutexUnlock( &config->lock );
          return COSM_PASS;
        } /* key matched */
      } /* section matched */
      /* section exists, key does not, alloc a new key */
      if ( tmp_section->key_count == 0 )
      {
        if ( ( tmp_section->keys = (cosm_CONFIG_KEY *) CosmMemAlloc(
          sizeof( cosm_CONFIG_KEY ) ) ) == NULL )
        {
          CosmMutexUnlock( &config->lock );
          return COSM_FAIL;
        }
        tmp_section->key_count = 1;
      }
      else
      {
        if ( ( tmp_section->keys = (cosm_CONFIG_KEY *)
          CosmMemRealloc( tmp_section->keys, (u64)
          ( tmp_section->key_count + 1 ) * sizeof( cosm_CONFIG_KEY ) ) )
          == NULL )
        {
          CosmMutexUnlock( &config->lock );
          return COSM_FAIL;
        }
        CosmMemSet( &tmp_section->keys[tmp_section->key_count],
          sizeof( cosm_CONFIG_KEY ), 0 );
        tmp_section->key_count += 1;
      }
      /* make another key struct and add it in */
      j = tmp_section->key_count - 1;
      tmp_section->keys[j].length = length;
      if ( ( tmp_section->keys[j].key =
         CosmMemAlloc( (u64) length ) ) == NULL )
      {
        tmp_section->key_count -= 1;
        CosmMutexUnlock( &config->lock );
        return COSM_FAIL;
      }
      CosmStrCopy( tmp_section->keys[j].key, key, key_len );
      tmp_section->keys[j].value =
         CosmMemOffset( tmp_section->keys[j].key, key_len );
      CosmStrCopy( tmp_section->keys[j].value, value, val_len );
      tmp_section->keys[j].flag = COSM_CONFIG_ALLOCATED;

      CosmMutexUnlock( &config->lock );
      return COSM_PASS;
    } /* section matched */
  } /* section loop */
  /* section does not exist, alloc a new one and create a key */
  if ( config->section_count == 0 )
  {
    if ( ( config->sections = (cosm_CONFIG_SECTION *)
      CosmMemAlloc( sizeof( cosm_CONFIG_SECTION ) ) ) == NULL )
    {
      CosmMutexUnlock( &config->lock );
      return COSM_FAIL;
    }
    config->section_count = 1;
  }
  else
  {
    if ( ( config->sections = (cosm_CONFIG_SECTION *)
      CosmMemRealloc( config->sections, (u64)
      ( config->section_count + 1 ) * sizeof( cosm_CONFIG_SECTION ) ) )
      == NULL )
    {
      CosmMutexUnlock( &config->lock );
      return COSM_FAIL;
    }
    CosmMemSet( &config->sections[config->section_count],
      sizeof( cosm_CONFIG_SECTION ), 0 );
    config->section_count += 1;
  }
  j = config->section_count - 1;
  config->sections[j].flag = COSM_CONFIG_ALLOCATED;
  config->sections[j].length = CosmStrBytes( section ) + 1;
  if ( ( config->sections[j].section = CosmMemAlloc(
    (u64) config->sections[j].length ) ) == NULL )
  {
    CosmMutexUnlock( &config->lock );
    return COSM_FAIL;
  }
  CosmStrCopy( config->sections[j].section, section,
    config->sections[j].length );
  if ( ( config->sections[j].keys = (cosm_CONFIG_KEY *) CosmMemAlloc(
    sizeof( cosm_CONFIG_KEY ) ) ) == NULL )
  {
    CosmMutexUnlock( &config->lock );
    return COSM_FAIL;
  }
  config->sections[j].key_count = 1;

  tmp_section = &config->sections[j];
  tmp_section->keys[0].length = length;
  if ( ( tmp_section->keys[0].key =
     CosmMemAlloc( (u64) length ) ) == NULL )
  {
    CosmMutexUnlock( &config->lock );
    return COSM_FAIL;
  }
  CosmStrCopy( tmp_section->keys[0].key, key, key_len );
  tmp_section->keys[0].value =
     CosmMemOffset( tmp_section->keys[0].key, key_len );
  CosmStrCopy( tmp_section->keys[0].value, value, val_len );
  tmp_section->keys[0].flag = COSM_CONFIG_ALLOCATED;

  CosmMutexUnlock( &config->lock );
  return COSM_PASS;
}

const utf8 * CosmConfigGet( cosm_CONFIG * config, const utf8 * section,
  const utf8 * key )
{
  u32 i, j;
  cosm_CONFIG_SECTION * tmp_section;

  if ( config == NULL )
  {
    return NULL;
  }

  if ( CosmMutexLock( &config->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return NULL;
  }

  for ( i = 0 ; i < config->section_count; i++ )
  {
    if ( CosmStrCmp( section, config->sections[i].section,
      CosmStrBytes( section ) ) == 0 )
    {
      /* found the section */
      tmp_section = &config->sections[i];
      for ( j = 0 ; j < tmp_section->key_count; j++ )
      {
        if ( ( CosmStrCmp( key, tmp_section->keys[j].key, CosmStrBytes( key ) )
          == 0 ) && ( ( tmp_section->keys[j].flag == COSM_CONFIG_NORMAL )
          || ( tmp_section->keys[j].flag == COSM_CONFIG_ALLOCATED ) ) )
        {
          CosmMutexUnlock( &config->lock );
          return tmp_section->keys[j].value;
        }
      }
    }
  }

  CosmMutexUnlock( &config->lock );
  return NULL;
}

void CosmConfigFree( cosm_CONFIG * config )
{
  u32 i, j;
  cosm_CONFIG_SECTION * section;

  if ( config == NULL )
  {
    return;
  }

  if ( CosmMutexLock( &config->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return;
  }

  for ( i = 0 ; i < config->section_count; i++ )
  {
    section = &config->sections[i];
    for ( j = 0 ; j < section->key_count; j++ )
    {
      if ( section->keys[j].flag == COSM_CONFIG_ALLOCATED )
      {
        CosmMemFree( section->keys[j].key );
      }
    }
    if ( section->flag == COSM_CONFIG_ALLOCATED )
    {
      CosmMemFree( section->section );
    }
    CosmMemFree( section->keys );
  }
  CosmMemFree( config->sections );
  CosmMemFree( config->memory );

  config->memory = NULL;
  config->sections = NULL;
  config->section_count = 0;

  CosmMutexUnlock( &config->lock );
  CosmMutexFree( &config->lock );
}

s32 Cosm_TestConfig( void )
{
  cosm_CONFIG conf;
  const utf8 * ptr;

  CosmMemSet( &conf, sizeof( cosm_CONFIG ), 0 );

  if ( CosmConfigLoad( &conf, NULL ) != COSM_PASS )
  {
    return -1;
  }

  /* set some keys */
  if ( CosmConfigSet( &conf, "s1", "k1",
    "v11" ) != COSM_PASS )
  {
    return -2;
  }
  if ( CosmConfigSet( &conf, "s1", "k2",
    "v12" ) != COSM_PASS )
  {
    return -3;
  }
  if ( CosmConfigSet( &conf, "s2", "k1",
    "v21" ) != COSM_PASS )
  {
    return -4;
  }
  if ( CosmConfigSet( &conf, "s2", "k2",
    "v22" ) != COSM_PASS )
  {
    return -5;
  }

  /* check em */
  ptr = CosmConfigGet( &conf, "s1", "k1" );
  if ( CosmStrCmp( ptr, "v11", 4 ) )
  {
    return -6;
  }
  ptr = CosmConfigGet( &conf, "s1", "k2" );
  if ( CosmStrCmp( ptr, "v12", 4 ) )
  {
    return -7;
  }
  ptr = CosmConfigGet( &conf, "s2", "k1" );
  if ( CosmStrCmp( ptr, "v21", 4 ) )
  {
    return -8;
  }
  ptr = CosmConfigGet( &conf, "s2", "k2" );
  if ( CosmStrCmp( ptr, "v22", 4 ) )
  {
    return -9;
  }

  /* change and check some */
  if ( CosmConfigSet( &conf, "s1", "k1",
    "v11longer" ) != COSM_PASS )
  {
    return -10;
  }
  ptr = CosmConfigGet( &conf, "s1", "k1" );
  if ( CosmStrCmp( ptr, "v11longer", 10 ) )
  {
    return -11;
  }
  if ( CosmConfigSet( &conf, "s2", "k2",
    "v" ) != COSM_PASS )
  {
    return -12;
  }
  ptr = CosmConfigGet( &conf, "s2", "k2" );
  if ( CosmStrCmp( ptr, "v", 2 ) )
  {
    return -13;
  }

  /* save it and free */
  if ( CosmConfigSave( &conf, "test.cfg" ) != COSM_PASS )
  {
    return -14;
  }
  CosmConfigFree( &conf );

  /* load it again (and delete the file) */
  if ( CosmConfigLoad( &conf, "test.cfg" ) != COSM_PASS )
  {
    return -15;
  }
  CosmFileDelete( "test.cfg" );

  /* check the loaded version and free it */
  ptr = CosmConfigGet( &conf, "s1", "k1" );
  if ( CosmStrCmp( ptr, "v11longer", 10 ) )
  {
    return -16;
  }
  ptr = CosmConfigGet( &conf, "s1", "k2" );
  if ( CosmStrCmp( ptr, "v12", 4 ) )
  {
    return -17;
  }
  ptr = CosmConfigGet( &conf, "s2", "k1" );
  if ( CosmStrCmp( ptr, "v21", 4 ) )
  {
    return -18;
  }
  ptr = CosmConfigGet( &conf, "s2", "k2" );
  if ( CosmStrCmp( ptr, "v", 2 ) )
  {
    return -19;
  }

  /* change some loaded ones */
  if ( CosmConfigSet( &conf, "s1", "k1",
    "v11" ) != COSM_PASS )
  {
    return -20;
  }
  ptr = CosmConfigGet( &conf, "s1", "k1" );
  if ( CosmStrCmp( ptr, "v11", 4 ) )
  {
    return -21;
  }
  if ( CosmConfigSet( &conf, "s2", "k2",
    "v22" ) != COSM_PASS )
  {
    return -22;
  }
  ptr = CosmConfigGet( &conf, "s2", "k2" );
  if ( CosmStrCmp( ptr, "v22", 4 ) )
  {
    return -23;
  }

  /* delete a key */
  if ( CosmConfigSet( &conf, "s1", "k1", NULL ) != COSM_PASS )
  {
    return -24;
  }
  ptr = CosmConfigGet( &conf, "s1", "k1" );
  if ( ptr != NULL )
  {
    return -25;
  }

  CosmConfigFree( &conf );

  return COSM_PASS;
}
