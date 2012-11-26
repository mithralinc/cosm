/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - 2nd Party Libary Wrappers

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "cosm/bzip2/bzip2.h"
#include "bzlib.h"
#include "cosm/os_mem.h"

#define COSM_BZIP2_TMP_SIZE   0x00010000  /* 64 KiB */
#define COSM_BZIP2_MAX_CHUNK  0x40000000  /* 1 GiB */

/* bzip2 hooks */

s32 Cosm_BZIP2CInit( cosm_TRANSFORM * transform, va_list params )
{
  s32 result;
  u32 level;

  level = va_arg( params, u32 );

  /* we need an outlet */
  if ( transform->next_transform == NULL )
  {
    return COSM_TRANSFORM_ERROR_NEXT;
  }

  /* level valid? */
  if ( ( level < 1 ) || ( level > 9 ) )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  transform->tmp_data = CosmMemAlloc( sizeof( bz_stream ) );
  if ( transform->tmp_data == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  if ( ( result = BZ2_bzCompressInit( transform->tmp_data, level, 0, 0 ) )
    != BZ_OK )
  {
    switch ( result )
    {
      case BZ_MEM_ERROR:
        CosmMemFree( transform->tmp_data );
        return COSM_TRANSFORM_ERROR_MEMORY;
      case BZ_CONFIG_ERROR:
      case BZ_PARAM_ERROR:
      default:
        CosmMemFree( transform->tmp_data );
        return COSM_TRANSFORM_ERROR_FATAL;
    }
  }

  return COSM_PASS;
}

s32 Cosm_BZIP2Comp( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  bz_stream * stream;
  u8 * tmp_out;
  u8 * ptr;
  u32 chunk;
  s32 result;

  tmp_out = CosmMemAlloc( (u64) COSM_BZIP2_TMP_SIZE );
  if ( tmp_out == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  ptr = (u8 *) data;

  while ( length > 0 )
  {
    if ( length > (u64) COSM_BZIP2_MAX_CHUNK )
    {
      chunk = COSM_BZIP2_MAX_CHUNK;
    }
    else
    {
      chunk = (u32) length;
    }

    stream = transform->tmp_data;
    stream->next_in = (char *) ptr;
    stream->avail_in = chunk;
    stream->next_out = (char *) tmp_out;
    stream->avail_out = COSM_BZIP2_TMP_SIZE;

    while ( stream->avail_in != 0 )
    {
      if ( BZ2_bzCompress( stream, BZ_RUN ) != BZ_RUN_OK )
      {
        CosmMemFree( tmp_out );
        return COSM_TRANSFORM_ERROR_FATAL;
      }

      /* we need to feed the data to the next transform if there is one */
      if ( transform->next_transform != NULL )
      {
        if ( ( result = CosmTransform( transform->next_transform, tmp_out,
          (u64) COSM_BZIP2_TMP_SIZE - stream->avail_out ) ) != COSM_PASS )
        {
          CosmMemFree( tmp_out );
          return result;
        }
      }
      stream->next_out = (char *) tmp_out;
      stream->avail_out = COSM_BZIP2_TMP_SIZE;
    }

    ptr = CosmMemOffset( ptr, (u64) chunk );
    length -= (u64) chunk;
  }

  CosmMemFree( tmp_out );

  return COSM_PASS;
}

s32 Cosm_BZIP2CEnd( cosm_TRANSFORM * transform )
{
  u8 * tmp_out;
  bz_stream * stream;
  s32 result, sub_result;

  tmp_out = CosmMemAlloc( (u64) COSM_BZIP2_TMP_SIZE );
  if ( tmp_out == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  stream = transform->tmp_data;
  stream->next_in = NULL;
  stream->avail_in = 0;
  stream->next_out = (char *) tmp_out;
  stream->avail_out = COSM_BZIP2_TMP_SIZE;

  do
  {
    result = BZ2_bzCompress( stream, BZ_FINISH );
    if ( ( result != BZ_FINISH_OK ) && ( result != BZ_STREAM_END ) )
    {
      CosmMemFree( tmp_out );
      return COSM_TRANSFORM_ERROR_FATAL;
    }

    /* we need to feed the data to the next transform if there is one */
    if ( transform->next_transform != NULL )
    {
      if ( ( sub_result = CosmTransform( transform->next_transform, tmp_out,
        (u64) COSM_BZIP2_TMP_SIZE - stream->avail_out ) ) != COSM_PASS )
      {
        CosmMemFree( tmp_out );
        return sub_result;
      }
    }

    stream->next_out = (char *) tmp_out;
    stream->avail_out = COSM_BZIP2_TMP_SIZE;
  } while ( result != BZ_STREAM_END );

  if ( BZ2_bzCompressEnd( stream ) != BZ_OK )
  {
    CosmMemFree( tmp_out );
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  CosmMemFree( tmp_out );

  return COSM_PASS;
}

s32 Cosm_BZIP2DInit( cosm_TRANSFORM * transform, va_list params )
{
  s32 result;

  /* we need an outlet */
  if ( transform->next_transform == NULL )
  {
    return COSM_TRANSFORM_ERROR_NEXT;
  }

  transform->tmp_data = CosmMemAlloc( sizeof( bz_stream ) );
  if ( transform->tmp_data == NULL )
  {
    return COSM_TRANSFORM_ERROR_STATE;
  }

  if ( ( result = BZ2_bzDecompressInit( transform->tmp_data, 0, 0 ) )
    != BZ_OK )
  {
    switch ( result )
    {
      case BZ_MEM_ERROR:
        CosmMemFree( transform->tmp_data );
        return COSM_TRANSFORM_ERROR_MEMORY;
      case BZ_CONFIG_ERROR:
      case BZ_PARAM_ERROR:
      default:
        CosmMemFree( transform->tmp_data );
        return COSM_TRANSFORM_ERROR_FATAL;
    }
  }

  return COSM_PASS;
}

s32 Cosm_BZIP2Decomp( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  bz_stream * stream;
  u8 * tmp_out;
  u8 * ptr;
  s32 result, sub_result;
  u32 chunk;

  tmp_out = CosmMemAlloc( (u64) COSM_BZIP2_TMP_SIZE );
  if ( tmp_out == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  ptr = (u8 *) data;

  while ( length > 0 )
  {
    if ( length > (u64) COSM_BZIP2_MAX_CHUNK )
    {
      chunk = COSM_BZIP2_MAX_CHUNK;
    }
    else
    {
      chunk = (u32) length;
    }

    stream = transform->tmp_data;
    stream->next_in = (char *) ptr;
    stream->avail_in = chunk;
    stream->next_out = (char *) tmp_out;
    stream->avail_out = COSM_BZIP2_TMP_SIZE;

    do
    {
      result = BZ2_bzDecompress( stream );
      if ( ( result != BZ_OK ) && ( result != BZ_STREAM_END ) )
      {
        CosmMemFree( tmp_out );
        return COSM_TRANSFORM_ERROR_FATAL;
      }

      /* we need to feed the data to the next transform if there is one */
      if ( transform->next_transform != NULL )
      {
        if ( ( sub_result = CosmTransform( transform->next_transform, tmp_out,
          (u64) COSM_BZIP2_TMP_SIZE - stream->avail_out ) ) != COSM_PASS )
        {
          CosmMemFree( tmp_out );
          return sub_result;
        }
      }

      stream->next_out = (char *) tmp_out;
      stream->avail_out = COSM_BZIP2_TMP_SIZE;
    } while ( ( stream->avail_in != 0 ) && ( result != BZ_STREAM_END ) );

    ptr = CosmMemOffset( ptr, (u64) chunk );
    length -= (u64) chunk;
  }

  CosmMemFree( tmp_out );

  return COSM_PASS;
}

s32 Cosm_BZIP2DEnd( cosm_TRANSFORM * transform )
{
  if ( BZ2_bzDecompressEnd( transform->tmp_data ) != BZ_OK )
  {
    CosmMemFree( transform->tmp_data );
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}
