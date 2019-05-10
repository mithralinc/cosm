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
