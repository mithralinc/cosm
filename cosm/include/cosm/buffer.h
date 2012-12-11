/*
  Copyright 1995-2012 Mithral Communications & Design Inc.

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

#ifndef COSM_BUFFER_H
#define COSM_BUFFER_H

#include "cosm/cputypes.h"
#include "cosm/os_task.h"

typedef struct cosm_BUFFER
{
  u64 data_length;
  u64 mem_length;
  u64 grow_size;
  u64 head;
  u64 tail;
  void * memory;
  u32 mode;
} cosm_BUFFER;

/*
  Notes:
  Buffers never have to worry about secure memory situations
  Buffers are implemented as circular buffers
*/

#define COSM_BUFFER_MODE_NONE      0
#define COSM_BUFFER_MODE_QUEUE     1 /* FIFO */
#define COSM_BUFFER_MODE_STACK     2 /* FILO */

#define COSM_BUFFER_ERROR_MEMORY  -1 /* Not enough memory */
#define COSM_BUFFER_ERROR_FULL    -2 /* Buffer is full */
#define COSM_BUFFER_ERROR_MODE    -3 /* Unknown mode */
#define COSM_BUFFER_ERROR_PARAM   -4 /* Invalid parameter */


s32 CosmBufferInit( cosm_BUFFER * buffer, u64 size, u32 mode, u64 grow,
  const void * const data, u64 length );
  /*
    Setup the cosm_BUFFER struct and do an initial memory allocation of size
    bytes save alot of reallocs later. (i.e. good chance to boost performance)
    grow is the amount to increase the buffer by when more space is needed.
    If grow is zero, it will not increase and may return a FULL error.
    If data is non-NULL then copy length bytes of initial data into buffer.
    Returns: COSM_PASS on success, or an error code on failure.
  */

u64 CosmBufferLength( cosm_BUFFER * buffer );
  /*
    Returns: the length of the data currently in the buffer.
  */

s32 CosmBufferClear( cosm_BUFFER * buffer );
  /*
    Sets the length of the buffer to 0 without freeing it.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmBufferPut( cosm_BUFFER * buffer, const void * data, u64 length );
  /*
    Insert length bytes of data into the buffer.
    Returns: COSM_PASS on success, or an error code on failure.
  */

u64 CosmBufferGet( void * data, u64 length, cosm_BUFFER * buffer );
  /*
    Remove up to length bytes of data from the buffer.
    Returns: length on success, otherwise bytes read out of the buffer.
  */

s32 CosmBufferUnget( cosm_BUFFER * buffer, const void * data, u64 length );
  /*
    Insert length bytes of data back into the buffer after it was taken out
    with a CosmBufferGet().
    We shouldn't need to warn you about using this function without thinking
    in a multithreaded environment.
    Returns: COSM_PASS on success, or an error code on failure.
  */

void CosmBufferFree( cosm_BUFFER * buffer );
  /*
    Free all data in the buffer and return it to an uninitialized state.
    Returns: nothing.
  */

/* Low level function. */

s32 Cosm_BufferGrow( cosm_BUFFER * buffer, u64 length );
  /*
    Grow the buffer's memory to deal with length more bytes.
    Returns: COSM_PASS on success, or an error code on failure
  */

s32 Cosm_BufferPut( cosm_BUFFER * buffer, const void * data, u64 length );
  /*
    This function does the work for CosmBufferPut, except for no locking, also
    called from CosmBufferInit.
    Returns: COSM_PASS on success, or an error code on failure
  */

/* testing */

s32 Cosm_TestBuffer( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
