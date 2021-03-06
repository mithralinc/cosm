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

/* CPU/OS Layer - CPU and OS specific code is allowed */

#ifndef COSM_OS_MEM_H
#define COSM_OS_MEM_H

#include "cosm/cputypes.h"

#if ( !defined( MEM_LEAK_FIND ) )
#define CosmMemAlloc Cosm_MemAlloc
#define CosmMemAllocSecure Cosm_MemAllocSecure
#define CosmMemRealloc Cosm_MemRealloc
#define CosmMemFree Cosm_MemFree
#else
#define CosmMemAlloc( bytes ) \
  Cosm_MemAllocLeak( bytes, __FILE__, __LINE__ )
#define CosmMemAllocSecure( bytes ) \
  Cosm_MemAllocSecureLeak( bytes, __FILE__, __LINE__ )
#define CosmMemRealloc( memory, bytes ) \
  Cosm_MemReallocLeak( memory, bytes, __FILE__, __LINE__ )
#define CosmMemFree( memory ) \
  Cosm_MemFreeLeak( memory, __FILE__, __LINE__ )
#define CosmMemDumpLeaks( filename ) \
  Cosm_MemDumpLeaks( filename, __FILE__, __LINE__ );
#endif /* MEM_LEAK_FIND */

s32 CosmMemCopy( void * dest, const void * src, u64 length );
  /*
    Copy length bytes of memory from src to dest.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmMemSet( void * memory, u64 length, u8 value );
  /*
    Set length bytes of memory to value.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmMemCmp( const void * blockA, const void * blockB, u64 max_bytes );
  /*
    Compare the memory blocks blockA and blockB, up to max_bytes will
    be compared.
    Returns: 0 if the blocks are identical. If they are different then a
      positive or negative based on memA[x] - memB[x]. Due to endian issues
      the +/- result may or may not be relivant. If any parameter is NULL/0
      then it returns -1 unless blockA == blockB.
  */

void * CosmMemOffset( const void * memory, u64 offset );
  /*
    Calculates the address of memory+offset.
    Returns: pointer to the data location, NULL if address space is exceeded.
  */

s32 CosmMemSystem( u64 * amount );
  /*
    Set amount to the number of bytes of physical RAM in the system.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* low level */

void * Cosm_MemAlloc( u64 bytes );
  /*
    Allocate a block of zeroed memory. Will allocate 16*floor((bytes+15)/16))
    bytes, i.e. blocks of 16 bytes, and only on 16 byte boundries.
    Returns: An aligned pointer to the RAM block, or NULL on error.
  */

void * Cosm_MemAllocSecure( u64 bytes );
  /*
    Allocate a block of zeroed memory. Will allocate 16*floor((bytes+15)/16))
    bytes, i.e. blocks of 16 bytes, and only on 16 byte boundries.
    Attempts will be made to:
      - The memory will be protected from read or write from any other
        process including supervisor run programs or other processes owned
        by the same user, unless explicitly given access by the process.
      - The memory will not be swapped or otherwise copied out of live RAM
        including during a crash triggered dump.
      - The memory will not be shared to another machine.
      - The parts of the kernel in supervisor space will not be modified
        after boot (which would allow access).
    Never reallocate secure memory, as it will lose these protections.
    Many systems do not support exnhanced security memory at all, or for
    non-root/Administrator programs, which is the cause of nearly all security
    problems.
    Returns: An aligned pointer to the RAM block, or NULL on error.
  */

void * Cosm_MemRealloc( void * memory, u64 bytes );
  /*
    Adjust the size of the ram block to bytes length, copying any old data.
    The secure flag will be treated the same as it is in CosmMemAlloc above.
    Memory should also be aligned on 16 byte boundries similar to
    CosmMemAlloc. If memory is NULL, this function acts like CosmMemAlloc.
    If bytes is zero, then free the memory, and return NULL.
    Any expanded memory space will have unknown values.
    [ideally we would zero new/old memory in the process]
    Returns: A pointer to the new memory space on success, or NULL if unable
      to change the size - leaving the old memory unmodified.
  */

void Cosm_MemFree( void * memory );
  /*
    Free the allocated memory. memory may safely be NULL.
    Memory allocated with CosmMemAllocSecure will be overwritten.
    Returns: nothing.
  */

void * Cosm_MemAllocLeak( u64 bytes, const ascii * file, u32 line );
  /*
    Leak finding version of Cosm_MemAlloc.
    Returns: An aligned pointer to the RAM block, or NULL on error.
  */

void * Cosm_MemAllocSecureLeak( u64 bytes, const ascii * file, u32 line );
  /*
    Leak finding version of Cosm_MemAllocSecure.
    Returns: An aligned pointer to the RAM block, or NULL on error.
  */

void * Cosm_MemReallocLeak( void * memory, u64 bytes,
  const ascii * file, u32 line );
  /*
    Leak finding version of Cosm_MemRealloc.
    Returns: A pointer to the new memory space on success, or NULL if unable
      to change the size (the old memory will not be modified).
  */

void Cosm_MemFreeLeak( void * memory, const ascii * file, u32 line );
  /*
    Leak finding version of Cosm_MemFree.
    Returns: nothing.
  */

s32 Cosm_MemDumpLeaks( const ascii * filename, const ascii * file, u32 line );
  /*
    Write out the allocated memory to the file filename.
    This function only exists if MEM_LEAK_FIND is defined.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* testing */

s32 Cosm_TestOSMem( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
