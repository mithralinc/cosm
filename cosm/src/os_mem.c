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

/* CPU/OS Layer - CPU and OS specific code is allowed */

#include "cosm/cputypes.h"
#include "cosm/os_mem.h"
#include "cosm/os_math.h"
#include "cosm/os_io.h"

#include <stdlib.h>
#include <string.h> /* for memmove */

#if ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_ANDROID ) )
#include <sys/sysinfo.h>
#elif ( ( OS_TYPE == OS_MACOSX ) || ( OS_TYPE == OS_OPENBSD ) \
  || ( OS_TYPE == OS_NETBSD ) )
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if ( defined( SYSV_SHARED_MEM ) )
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

/* Memory leak related defines */
#include "cosm/os_file.h"

#define COSM_LEAK_SENTINEL 0x3F6A8885A308D313LL
#define COSM_LEAK_PADDING  16

typedef struct cosm_MEMORY_LEAK
{
  cosm_FILENAME file;
  cosm_FILENAME refile;
  u64 size;
  u32 line;
  u32 reline;
  void * fake_memory;
  void * real_memory;
} cosm_MEMORY_LEAK;

cosm_MUTEX memory_leak_mutex;
u32 memory_leak_init = 0;
cosm_MEMORY_LEAK * memory_leaks = NULL;
u32 memory_leak_count = 0;
u32 memory_leak_alloc = 0;

s32 CosmMemCopy( void * dest, const void * src, u64 length )
{
  /* Fail if either dest or src is NULL */
  if ( ( dest == NULL ) || ( src == NULL ) )
  {
    return COSM_FAIL;
  }

  /* Do nothing if dest == src, or length is zero */
  if ( ( dest == src ) || ( ( length ==  0 ) ) )
  {
    return COSM_PASS;
  }

#if ( defined( CPU_64BIT ) )
  memmove( dest, src, length );
#else /* 32bits */
  if ( length > 0xFFFFFFFFLL )
  {
    return COSM_FAIL;
  }
  memmove( dest, src, (u32) length );
#endif

  return COSM_PASS;
}

s32 CosmMemSet( void * memory, u64 length, u8 value )
{
  if ( memory == NULL )
  {
    return COSM_FAIL;
  }

#if ( defined( CPU_64BIT ) )
  memset( memory, value, length );
#else /* 32bits */
  if ( length > 0xFFFFFFFFLL )
  {
    return COSM_FAIL;
  }
  memset( memory, value, (u32) length );
#endif

  return COSM_PASS;
}

s32 CosmMemCmp( const void * blockA, const void * blockB, u64 max_bytes )
{
  u64 i;
  s32 a, b;
  u8 * pA, * pB;

  if ( ( blockA == NULL ) || ( blockB == NULL )
    || ( ( max_bytes ==  0 ) ) )
  {
    if ( blockA != blockB )
    {
      return -1;
    }

    return 0;
  }

  pA = (u8 *) blockA;
  pB = (u8 *) blockB;
  i = 0x0000000000000000LL;
  do
  {
    a = (s32) *(pA)++;
    b = (s32) *(pB)++;
    i++;
  } while ( ( a == b ) && ( i < max_bytes ) );

  return a - b;
}

void * CosmMemOffset( const void * memory, u64 offset )
{
  u8 * tmp;

  /*
    We let the compiler do the pointer math correctly, but then check for
    wrapping the only way we can.
  */
  if ( memory == NULL )
  {
    return NULL;
  }

  tmp = (u8 *) memory;

#if ( defined( CPU_64BIT ) )
  tmp = &tmp[offset];
  if ( (u64) tmp < (u64) memory )
  {
    return NULL; /* wrap */
  }
#else /* 32bit */
  if ( offset > 0xFFFFFFFFLL )
  {
    return NULL;
  }
  tmp = &tmp[(u32) offset];
  if ( (void *) tmp < (void *) memory )
  {
    return NULL; /* wrap */
  }
#endif

  return (void *) tmp;
}

s32 CosmMemSystem( u64 * amount )
{
  /*
    We must find out how much as a normal user and without any 'tricks'
    like reading /proc or /dev.
  */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* this has some major limitations (2GiB limit) */
  MEMORYSTATUSEX info;

  info.dwLength = sizeof( info );
  if ( 0 == GlobalMemoryStatusEx( &info ) )
  {
    *amount = GetLastError();
    return COSM_FAIL;
  }
  *amount = info.ullTotalPhys;
  return COSM_PASS;
#elif ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_ANDROID ) )
  struct sysinfo info;

  CosmMemSet( &info, sizeof( info ), 0 );
  if ( sysinfo( &info ) != -1 )
  {
    *amount = (u64) info.totalram;
    if ( info.mem_unit > 0 ) /* kernel 2.3.23+ */
    {
      /* 4GiB of RAM or more possible if mem_unit is set */
      *amount = *amount * (u64) info.mem_unit;
    }
    return COSM_PASS;
  }
#elif ( OS_TYPE == OS_MACOSX )
#ifndef HW_MEMSIZE
#define HW_MEMSIZE 24
#endif
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  u32 mem32;
  u64 mem64;
  size_t length;

  /* try u64 first */
  length = sizeof( mem64 );
  if ( sysctl( mib, 2, &mem64, &length, NULL, 0 ) == -1 )
  {
    /* failback to u32 */
    mib[1] = HW_PHYSMEM;
    length = sizeof( mem32 );
    if ( sysctl( mib, 2, &mem32, &length, NULL, 0 ) == -1 )
    {
      return COSM_FAIL;
    }
    *amount = (u64) mem32;
  }
  *amount = mem64;

  return COSM_PASS;
#elif ( ( OS_TYPE == OS_OPENBSD ) || ( OS_TYPE == OS_NETBSD ) )
  int mib[2], mem;
  size_t len;

  mib[0] = CTL_HW;
  mib[1] = HW_PHYSMEM;
  len = sizeof( mem );

  if ( sysctl( mib, 2, &mem, &len, NULL, 0 ) != -1 )
  {
    *amount = (u64) mem;
    return COSM_PASS;
  }
#endif

  return COSM_FAIL;
}

/* low level */

void * Cosm_MemAlloc( u64 bytes )
{
  u64 align;
  u64 request;

  /* Return NULL if bytes is 0 */
  if ( bytes == 0 )
  {
    return NULL;
  }

  /*
    we still need to find a way to make sure we return a
    16 byte aligned address, but it should be done already
  */
  align = 0x0000000000000010LL;
  request = ( bytes / align ) * align;
  if ( request != bytes )
  {
    request += align;
    /* integer overflow? */
    if ( request < align )
    {
      return NULL;
    }  
  }

#if ( defined( CPU_64BIT ) )
  return calloc( 1, request );
#else /* 32bits */
  if ( bytes > 0xFFFFFFFFLL )
  {
    return NULL;
  }
  return calloc( 1, (u32) request );
#endif
}

void * Cosm_MemAllocSecure( u64 bytes )
{
  /*
    Do as much as possible to make sure the RAM is secure. Some OSes
    will allow us to at least tighted security a little bit.
    Lock Memory into RAM, secure it from other processes, etc.
    So far mainstream OS security is a joke and we can't do much.
    Currently we cannot do anything about it unless we are root/Administrator
  */

  /* !!! */
  return Cosm_MemAlloc( bytes );
}

void * Cosm_MemRealloc( void * memory, u64 bytes )
{
  u64 align;
  u64 request;

  /* Free memory and return NULL if bytes is 0 and 'memory' is not NULL */
  if ( ( bytes == 0 ) && ( memory != NULL ) )
  {
    CosmMemFree( memory );
    return NULL;
  }

  if ( memory == NULL )
  {
    /* new allocation */
    return Cosm_MemAlloc( bytes );
  }

  /* alignment issues */
  align = 0x0000000000000010LL;
  request = ( bytes / align ) * align;
  if ( request != bytes )
  {
    request = ( request + align );
  }

#if ( defined( CPU_64BIT ) )
  return realloc( memory, request );
#else /* 32bits */
  if ( bytes > 0xFFFFFFFFLL )
  {
    return NULL;
  }
  return realloc( memory, (u32) request );
#endif
}

void Cosm_MemFree( void * memory )
{
  if ( memory != NULL )
  {
    free( memory );
  }
}

void * Cosm_MemAllocLeak( u64 bytes, const ascii * file, u32 line )
{
  u8 * mem;
  u64 pad_size;

  pad_size = bytes + COSM_LEAK_PADDING * 2;
  /* check for overflow */
  if ( pad_size < bytes )
  {
    return NULL;
  }

  if ( ( bytes == 0 ) || ( ( mem = Cosm_MemAlloc( pad_size ) ) == NULL ) )
  {
    return NULL;
  }

  if ( memory_leak_init == 0 )
  {
    CosmMemSet( &memory_leak_mutex, sizeof( cosm_MUTEX ), 0 );
    CosmMutexInit( &memory_leak_mutex );
    memory_leak_init = 1;
  }

  if ( CosmMutexLock( &memory_leak_mutex, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return NULL;
  }

  memory_leak_count++;

  if ( memory_leak_count > memory_leak_alloc )
  {
    memory_leak_alloc += 64;
    memory_leaks = Cosm_MemRealloc( memory_leaks,
      memory_leak_alloc * sizeof( cosm_MEMORY_LEAK ) );
  }

  CosmStrCopy( memory_leaks[memory_leak_count-1].file, file,
        (u64) COSM_FILE_MAX_FILENAME );
  memory_leaks[memory_leak_count-1].refile[0] = 0;
  memory_leaks[memory_leak_count-1].size = bytes;
  memory_leaks[memory_leak_count-1].line = line;
  memory_leaks[memory_leak_count-1].reline = 0;
  memory_leaks[memory_leak_count-1].real_memory = mem;
  memory_leaks[memory_leak_count-1].fake_memory = mem + COSM_LEAK_PADDING;

  CosmMutexUnlock( &memory_leak_mutex );

  *((u64 *) &mem[0]) = COSM_LEAK_SENTINEL;
  *((u64 *) &mem[8]) = COSM_LEAK_SENTINEL;
  *((u64 *) &mem[COSM_LEAK_PADDING + bytes]) = COSM_LEAK_SENTINEL;
  *((u64 *) &mem[COSM_LEAK_PADDING + bytes + 8]) = COSM_LEAK_SENTINEL;

  return ( mem + COSM_LEAK_PADDING );
}

void * Cosm_MemAllocSecureLeak( u64 bytes, const ascii * file, u32 line )
{
  /* see Cosm_MemAllocSecure for info */
  
  /* !!! */
  return Cosm_MemAllocLeak( bytes, file, line );  
}

void * Cosm_MemReallocLeak( void * memory, u64 bytes,
  const ascii * file, u32 line )
{
  u8 * mem;
  u32 i;
  u64 pad_size;

  /* Free memory and return NULL if bytes is 0 and 'memory' is not NULL */
  if ( ( bytes == 0 ) && ( memory != NULL ) )
  {
    Cosm_MemFreeLeak( (u8 *) memory - COSM_LEAK_PADDING, file, line );
    return NULL;
  }

  if ( memory == NULL )
  {
    /* no previous allocation */
    return Cosm_MemAllocLeak( bytes, file, line );
  }

  pad_size = bytes + COSM_LEAK_PADDING * 2;

  /* our real address is a back a bit */
  if ( ( mem = Cosm_MemRealloc( (u8 *) memory - COSM_LEAK_PADDING,
    pad_size ) ) == NULL )
  {
    return NULL;
  }

  if ( memory_leak_init == 0 )
  {
    CosmMemSet( &memory_leak_mutex, sizeof( cosm_MUTEX ), 0 );
    CosmMutexInit( &memory_leak_mutex );
    memory_leak_init = 1;
  }

  if ( CosmMutexLock( &memory_leak_mutex, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return NULL;
  }

  for ( i = 0 ; i < memory_leak_count ; i++ )
  {
    if ( memory_leaks[i].fake_memory == memory )
    {
      CosmStrCopy( memory_leaks[i].refile, file, COSM_FILE_MAX_FILENAME );
      memory_leaks[i].reline = line;
      memory_leaks[i].size = bytes;
      memory_leaks[i].real_memory = mem;
      memory_leaks[i].fake_memory = mem + COSM_LEAK_PADDING;
      CosmMutexUnlock( &memory_leak_mutex );

      /* fix up a new tail sentinel */
      *((u64 *) &mem[COSM_LEAK_PADDING + bytes]) = COSM_LEAK_SENTINEL;
      *((u64 *) &mem[COSM_LEAK_PADDING + bytes + 8]) = COSM_LEAK_SENTINEL;

      return ( mem + COSM_LEAK_PADDING );
    }
  }

  /* no match */
  memory_leak_count++;

  if ( memory_leak_count > memory_leak_alloc )
  {
    memory_leak_alloc += 64;
    memory_leaks = Cosm_MemRealloc( memory_leaks,
      memory_leak_alloc * sizeof( cosm_MEMORY_LEAK ) );
  }

  CosmStrCopy( memory_leaks[memory_leak_count-1].file, file,
    COSM_FILE_MAX_FILENAME );
  memory_leaks[memory_leak_count-1].refile[0] = 0;
  memory_leaks[memory_leak_count-1].size = bytes;
  memory_leaks[memory_leak_count-1].line = line;
  memory_leaks[memory_leak_count-1].reline = 0;
  memory_leaks[memory_leak_count-1].real_memory = mem;
  memory_leaks[memory_leak_count-1].fake_memory = mem + COSM_LEAK_PADDING;

  CosmMutexUnlock( &memory_leak_mutex );

  return ( mem + COSM_LEAK_PADDING );
}

void Cosm_MemFreeLeak( void * memory, const ascii * file, u32 line )
{
  u32 i;
  u8 * mem;

  if ( memory != NULL )
  {
    if ( memory_leak_init == 0 )
    {
      CosmMemSet( &memory_leak_mutex, sizeof( cosm_MUTEX ), 0 );
      CosmMutexInit( &memory_leak_mutex );
      memory_leak_init = 1;
    }

    if ( CosmMutexLock( &memory_leak_mutex, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return;
    }

    for ( i = 0 ; i < memory_leak_count ; i++ )
    {
      if ( memory_leaks[i].fake_memory == memory )
      {
        /* found it, test that it is not corrupted */
        mem = memory_leaks[i].real_memory;
        if ( ( COSM_LEAK_SENTINEL != *((u64 *) &mem[0]) )
          || ( COSM_LEAK_SENTINEL != *((u64 *) &mem[8]) )
          || ( COSM_LEAK_SENTINEL != 
          *((u64 *) &mem[COSM_LEAK_PADDING + memory_leaks[i].size]) )
          || ( COSM_LEAK_SENTINEL !=
          *((u64 *) &mem[COSM_LEAK_PADDING + memory_leaks[i].size + 8]) ) )
        {
          /* clobbered memory, at least attempt to let coder know */
          CosmPrint( "Memory corrupted, freed from %.*s line %u\n",
            COSM_FILE_MAX_FILENAME, file, line );
          CosmPrint( "Allocated: %p - %v bytes - %.*s line %u\n",
            memory_leaks[i].fake_memory, memory_leaks[i].size, COSM_FILE_MAX_FILENAME,
            memory_leaks[i].file, memory_leaks[i].line );
        }

        Cosm_MemFree( memory_leaks[i].real_memory );
        
        CosmMemCopy( &memory_leaks[i], &memory_leaks[--memory_leak_count],
          sizeof( cosm_MEMORY_LEAK ) );

        break;
      }
    }

    CosmMutexUnlock( &memory_leak_mutex );    
  }
}

s32 Cosm_MemDumpLeaks( const ascii * filename, const ascii * file, u32 line )
{
  cosm_FILE outfile;
  u32 i;
  u8 * mem;

  if ( filename == NULL )
  {
    return COSM_FAIL;
  }

  CosmMemSet( &outfile, sizeof( cosm_FILE ), 0 );

  if ( CosmFileOpen( &outfile, filename, COSM_FILE_MODE_WRITE
    | COSM_FILE_MODE_CREATE | COSM_FILE_MODE_TRUNCATE, COSM_FILE_LOCK_WRITE )
    != COSM_PASS )
  {
    return COSM_FAIL;
  }

  if ( memory_leak_init == 0 )
  {
    CosmMemSet( &memory_leak_mutex, sizeof( cosm_MUTEX ), 0 );
    CosmMutexInit( &memory_leak_mutex );
    memory_leak_init = 1;
  }
  if ( CosmMutexLock( &memory_leak_mutex, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  CosmPrintFile( &outfile,
    "Dump from %.*s line %u\nLeak count = %u\n\n",
    COSM_FILE_MAX_FILENAME, file, line, memory_leak_count );

  for ( i = 0 ; i < memory_leak_count ; i++ )
  {
    CosmPrintFile( &outfile, "%p - %v bytes - %.*s line %u\n",
      memory_leaks[i].fake_memory, memory_leaks[i].size, COSM_FILE_MAX_FILENAME,
      memory_leaks[i].file, memory_leaks[i].line );

    if ( ( memory_leaks[i].refile[0] != 0 )
      || ( memory_leaks[i].reline != 0 ) )
    {
      CosmPrintFile( &outfile, "  - reallocated %.*s line %u\n",
       COSM_FILE_MAX_FILENAME, memory_leaks[i].file, memory_leaks[i].reline );
    }

    mem = memory_leaks[i].real_memory;
    if ( ( COSM_LEAK_SENTINEL != *((u64 *) &mem[0]) )
      || ( COSM_LEAK_SENTINEL != *((u64 *) &mem[8]) )
      || ( COSM_LEAK_SENTINEL != 
      *((u64 *) &mem[COSM_LEAK_PADDING + memory_leaks[i].size]) )
      || ( COSM_LEAK_SENTINEL !=
      *((u64 *) &mem[COSM_LEAK_PADDING + memory_leaks[i].size + 8]) ) )
    {
      /* clobbered memory, at least attempt to let coder know */
      CosmPrintFile( &outfile, "  - MEMORY CORRUPTED!!!\n" );
    }
    else
    {
      CosmPrintFile( &outfile, "  - memory sentinels intact\n" );    
    }
  }

  CosmMutexUnlock( &memory_leak_mutex );

  CosmFileClose( &outfile );

  return COSM_PASS;
}

/* testing */

s32 Cosm_TestOSMem( void )
{
  u8 * ptr1, * ptr2;
  u8 * mem;
  u8 * offset;
  u32 i;
  u64 size;

  /* First be sure our COSM_MEM_SIZE is correct - set at the top of os_mem.c */
#if ( defined( CPU_64BIT ) )
  if ( sizeof( void * ) != 8 )
  {
    return -1;
  }
#else /* the 32bit case */
  if ( sizeof( void * ) != 4 )
  {
    return -1;
  }
#endif

  /* Next be sure our memory size matches the calloc size_t */
#if ( defined( CPU_64BIT ) )
  if ( sizeof( size_t ) != sizeof( u64 ) )
  {
    return -2;
  }
#else /* the 32bit case */
  if ( sizeof( size_t ) != sizeof( u32 ) )
  {
    return -2;
  }
#endif

  /*
    We simply try to allocate more memory then the machine
    can possibly be able to give up.
  */
#if ( defined( CPU_64BIT ) )
  size = 0xFFFFFFFFFFFFFFFFLL;
#else /* the 32bit case */
  size = 0x0000000100000000LL;
#endif
  if ( ( mem = (u8 *) CosmMemAlloc( size ) ) != NULL )
  {
    CosmMemFree( mem );
    return -3;
  }

  /* allocate 0 bytes */
  size = 0x0000000000000000LL;
  if ( ( mem = (u8 *) CosmMemAlloc( size ) ) != NULL )
  {
    CosmMemFree( mem );
    return -4;
  }

  /* test CosmMemAlloc */
  size = 0x0000000000000020LL;
  ptr1 = (u8 *) CosmMemAlloc( size );
  ptr2 = (u8 *) CosmMemAlloc( size );
  if ( ( ptr1 == NULL ) || ( ptr2 == NULL ) )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -5;
  }

  /* is it zeroed? */
  for ( i = 0 ; i < 0x20 ; i++ )
  {
    if ( ( ptr1[i] != 0 ) || ( ptr2[i] != 0 ) )
    {
      CosmMemFree( ptr1 );
      CosmMemFree( ptr2 );
      return -6;
    }
  }

  /* Put the number 0 .. 31 in  ptr1. */
  for ( i = 0 ; i < 0x20 ; i++ )
  {
    ptr1[i] = (u8) i;
  }

  /* Test CosmMemCopy, copy a bit of ptr1 to ptr2 */
  if ( CosmMemCopy( ptr2, ptr1, size ) == COSM_FAIL )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -7;
  }
  for ( i = 0 ; i < 0x20 ; i++ )
  {
    if ( ptr1[i] != ptr2[i] )
    {
      CosmMemFree( ptr1 );
      CosmMemFree( ptr2 );
      return -8;
    }
  }

  /* Test CosmMemRealloc */
  ptr1 = (u8 *) CosmMemRealloc( ptr1, 0x40LL );
  if ( ptr1 == NULL )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -9;
  }
  ptr1 = (u8 *) CosmMemRealloc( ptr1, 0x10LL );
  if ( ptr1 == NULL )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -10;
  }
  ptr1 = (u8 *) CosmMemRealloc( ptr1, 0x20LL );
  if ( ptr1 == NULL )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -11;
  }
  /* Only first 16 bytes must remain, last 16 can be lost in realloc */
  for ( i = 0 ; i < 0x10 ; i++ )
  {
    if ( ptr1[i] != ptr2[i] )
    {
      CosmMemFree( ptr1 );
      CosmMemFree( ptr2 );
      return -12;
    }
  }

  /* Test CosmMemSet */

  if ( CosmMemSet( ptr1, 0x08LL, (u8) 0xE7 ) != COSM_PASS )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -13;
  }
  /* Check first 8 bytes == 0xE7 */
  for ( i = 0 ; i < 0x08 ; i++ )
  {
    if ( ptr1[i] != (u8) 0xE7 )
    {
      CosmMemFree( ptr1 );
      CosmMemFree( ptr2 );
      return -14;
    }
  }
  /* Check last 8 bytes are the same */
  for ( i = 0x08 ; i < 0x10 ; i++ )
  {
    if ( ptr1[i] != ptr2[i] )
    {
      CosmMemFree( ptr1 );
      CosmMemFree( ptr2 );
      return -15;
    }
  }

  /* test CosmMemOffset */
  size = 0x000000000000000FLL;
  offset = (u8 *) CosmMemOffset( ptr2, size );
  if ( *offset != 0x0F )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -16;
  }
  size = 0x0000000000000014LL;
  offset = (u8 *) CosmMemOffset( ptr2, size );
  if ( *offset != 0x14 )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -17;
  }

  /* test CosmMemCmp */
  size = 0x0000000000000010LL;
  for ( i = 0 ; i < 0x10 ; i++ )
  {
    ptr1[i] = (u8) i;
    ptr2[i] = (u8) i;
  }
  if ( CosmMemCmp( ptr1, ptr2, size ) != 0 )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -18;
  }
  ptr1[6] = (u8) 42;
  if ( CosmMemCmp( ptr1, ptr2, size ) == 0 )
  {
    CosmMemFree( ptr1 );
    CosmMemFree( ptr2 );
    return -19;
  }

  /* No way to test CosmMemFree */
  CosmMemFree( ptr1 );
  CosmMemFree( ptr2 );

  return COSM_PASS;
}
