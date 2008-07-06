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

#ifndef COSM_TRANSFORM_H
#define COSM_TRANSFORM_H

#include "cputypes.h"
#include <stdarg.h>

#define COSM_TRANSFORM_COOKIE  0x42DEC0DE /* cookie to check for next */

#define COSM_TRANSFORM_STATE_INIT   42  /* Initialized */
#define COSM_TRANSFORM_STATE_DATA   43  /* Feeding */
#define COSM_TRANSFORM_STATE_DONE   44  /* Finished */
#define COSM_TRANSFORM_STATE_ERROR  45  /* Error, data invalid */

#define COSM_TRANSFORM_ERROR_PARAM   -1 /* A paramerter was invalid */
#define COSM_TRANSFORM_ERROR_STATE   -2 /* Invalid state, wrong order */
#define COSM_TRANSFORM_ERROR_MEMORY  -3 /* Memory problem */
#define COSM_TRANSFORM_ERROR_FATAL   -4 /* Fatal error, call ...End() */
#define COSM_TRANSFORM_ERROR_NEXT    -5 /* next_transform is not setup */

typedef struct cosm_TRANSFORM
{
  u32 cookie;
  u32 state;
  void * tmp_data;
  s32 (*init)( void *, va_list );
  s32 (*encode)( void *, const void * const, u64 );
  s32 (*end)( void * );
  struct cosm_TRANSFORM * next_transform;
} cosm_TRANSFORM;

s32 CosmTransformInit( cosm_TRANSFORM * transform,
  s32 (*init)( cosm_TRANSFORM *, va_list ),
  s32 (*encode)( cosm_TRANSFORM *, const void * const, u64 ),
  s32 (*end)( cosm_TRANSFORM * ),
  cosm_TRANSFORM * next_transform, ... );
  /*
    Setup and initialize the transform. params depends on the transform
    and is fed to the transform's init, which is called by this function.
    If next_transform is non-NULL, then the output is fed to that transform,
    and many transforms will require this.
    See the documentation for the transform functions, which are usually
    provided as a single define, for more information.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 CosmTransform( cosm_TRANSFORM * transform, const void * const data,
  u64 length );
  /*
    Feed length bytes of data into the transform.
    Note: Make sure your data has been CosmSave'd before using this.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 CosmTransformEnd( cosm_TRANSFORM * transform );
  /*
    Clear any temporary data and put the last of the transformed data out.
    If this function fails the transform is still cleaned up.
    Depending on the transform, other inputs/outputs will be available
    and need cleanup.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 CosmTransformEndAll( cosm_TRANSFORM * transform );
  /*
    Ends the transform and all transforms following it.
    Returns: COSM_PASS on success, or a transform error code on failure.
      Error returned will be the first error in the chain if there is one.
  */

/* low level base64 API */

s32 Cosm_Base64CInit( cosm_TRANSFORM * transform, va_list params );
  /*
    Allocate the temoprary data and initialize the compressor.
    params = none.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_Base64Comp( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Feed data to the compression engine, write any results to the buffer
    a line at a time - 76 chars + linefeed.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_Base64CEnd( cosm_TRANSFORM * transform );
  /*
    Flush any remaining data and free the temporary data.
    Fails if: deterministic, doesn't fail.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_Base64DInit( cosm_TRANSFORM * transform, va_list params );
  /*
    Allocate the temporary data and initialize the Decompressor.
    params = none.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_Base64Decomp( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Feed data to the decompression engine, write all results to the buffer.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_Base64DEnd( cosm_TRANSFORM * transform );
  /*
    Free the temporary data.
    Fails if: not enough input was read to finish up.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_BASE64_ENCODE \
  Cosm_Base64CInit, Cosm_Base64Comp, Cosm_Base64CEnd

#define COSM_BASE64_DECODE \
  Cosm_Base64DInit, Cosm_Base64Decomp, Cosm_Base64DEnd

/* very common transforms */

s32 Cosm_TransformToMemInit( cosm_TRANSFORM * transform, va_list params );
  /*
    params = (void *) to the start of a memory range.
    There is no length parameter, so do not use this transform unless
    you are absolutely sure you have allocated enough memory for the output.
    Since there are many transforms that will output either the same or less
    data then input (or tell you ahead of time), this is still reasonable.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToMem( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Write the data to memory.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToMemEnd( cosm_TRANSFORM * transform );
  /*
    Fails if: deterministic, doesn't fail.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_TRANSFORM_TO_MEMORY \
  Cosm_TransformToMemInit, Cosm_TransformToMem, Cosm_TransformToMemEnd

s32 Cosm_TransformToFileInit( cosm_TRANSFORM * transform, va_list params );
  /*
    params = (cosm_FILE *) to an already open file.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToFile( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Write the data to the file.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToFileEnd( cosm_TRANSFORM * transform );
  /*
    Fails if: deterministic, doesn't fail.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_TRANSFORM_TO_FILE \
  Cosm_TransformToFileInit, Cosm_TransformToFile, Cosm_TransformToFileEnd

s32 Cosm_TransformToNetInit( cosm_TRANSFORM * transform, va_list params );
  /*
    params = (cosm_NET *) to an already open netowrk connection.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToNet( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Write the data to the network.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToNetEnd( cosm_TRANSFORM * transform );
  /*
    Fails if: deterministic, doesn't fail.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_TRANSFORM_TO_NET \
  Cosm_TransformToNetInit, Cosm_TransformToNet, Cosm_TransformToNetEnd


s32 Cosm_TransformToBufferInit( cosm_TRANSFORM * transform, va_list params );
  /*
    params = (cosm_BUFFER *) to an already initialized buffer, which
    must be a queue not a stack.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToBuffer( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Write the data to the buffer.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

s32 Cosm_TransformToBufferEnd( cosm_TRANSFORM * transform );
  /*
    Fails if: deterministic, doesn't fail.
    Returns: COSM_PASS on success, or a transform error code on failure.
  */

#define COSM_TRANSFORM_TO_BUFFER \
  Cosm_TransformToBufferInit, Cosm_TransformToBuffer, \
  Cosm_TransformToBufferEnd

/* testing */

s32 Cosm_TestTransform( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
