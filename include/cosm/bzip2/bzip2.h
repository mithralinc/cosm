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

#ifndef COSM_BZIP2_H
#define COSM_BZIP2_H

#include "cosm/transform.h"

/* Low level bzip2 API */

s32 Cosm_BZIP2CInit( cosm_TRANSFORM * transform, va_list params );
  /*
    Allocate the temporary data and initialize the compressor.
    params = u32 to compression level.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_BZIP2Comp( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Feed data to the compression engine.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_BZIP2CEnd( cosm_TRANSFORM * transform );
  /*
    Flush any remaining data and free the temporary data.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_BZIP2DInit( cosm_TRANSFORM * transform, va_list params );
  /*
    Allocate the temporary data and initialize the decompressor.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_BZIP2Decomp( cosm_TRANSFORM * transform,
  const void * const data, u64 length );
  /*
    Feed data to the decompression engine.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_BZIP2DEnd( cosm_TRANSFORM * transform );
  /*
    Free the temporary data.
    Returns: COSM_PASS on success, or an error code on failure.
  */

#define COSM_BZIP2_COMPRESS \
  Cosm_BZIP2CInit, Cosm_BZIP2Comp, Cosm_BZIP2CEnd

#define COSM_BZIP2_DECOMPRESS \
  Cosm_BZIP2DInit, Cosm_BZIP2Decomp, Cosm_BZIP2DEnd

#endif
