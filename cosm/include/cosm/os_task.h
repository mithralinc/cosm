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

#ifndef COSM_OS_TASK_H
#define COSM_OS_TASK_H

#include "cosm/cputypes.h"
#include "cosm/os_math.h"

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#define _WIN32_WINNT 0x0501 /* User: XP+, Server: 2003+ */
#define WIN32_LEAN_AND_MEAN /* no old winsock */
#include <windows.h>
#include <signal.h>
#else /* OS */
#include <unistd.h>
#include <signal.h>
#if ( ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_NETBSD ) \
  || ( OS_TYPE == OS_OPENBSD ) || ( OS_TYPE == OS_FREEBSD ) ) \
  && ( !defined( _POSIX_THREADS ) ) ) /* forgot posix in headers */
#define _POSIX_THREADS
#endif /* system forgot define */
#if ( defined( _POSIX_THREADS ) )
#include <pthread.h>
#endif /* POSIX threads */
#endif /* OS */

/* These must match the native numberings */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#define COSM_SIGNAL_PING  0
#define COSM_SIGNAL_INT   SIGTERM /* these 2 are switched intentionally */
#define COSM_SIGNAL_TERM  SIGINT
#define COSM_SIGNAL_KILL  0x4242
#else
#define COSM_SIGNAL_PING  0
#define COSM_SIGNAL_INT   SIGINT
#define COSM_SIGNAL_TERM  SIGTERM
#define COSM_SIGNAL_KILL  SIGKILL
#endif

#define COSM_PRI_MAX 255  /* maximum thread priority */
#define COSM_PRI_MIN 1    /* minimum thread priority */

#define COSM_MUTEX_WAIT        15
#define COSM_MUTEX_NOWAIT      240
#define COSM_MUTEX_STATE_INIT  42

#define COSM_SEMAPHORE_WAIT        12
#define COSM_SEMAPHORE_NOWAIT      192
#define COSM_SEMAPHORE_STATE_INIT  47
#define COSM_SEMAPHORE_STATE_OPEN  511

typedef struct cosm_MUTEX
{
  u32 state;
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE os_mutex;
#elif ( defined( _POSIX_THREADS ) )
  pthread_mutex_t os_mutex;
#elif ( defined( SEM_VALUE_MAX ) )
  sem_t os_mutex;
#else
#error "no mutexes? check os_task.h"
#endif
} cosm_MUTEX;

#undef WINDOWS_SEMAPHORES
#undef POSIX_SEMAPHORES
#undef SYSV_SEMAPHORES
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#define WINDOWS_SEMAPHORES
/* if we have POSIX semaphores on UNIX we want to use them over Sys V */
#elif ( OS_TYPE == OS_MACOSX )
#include <semaphore.h>
#define POSIX_SEMAPHORES
#elif ( OS_TYPE == OS_LINUX )
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define SYSV_SEMAPHORES
#else
#error "Unknown semaphore types - see os_task.h"
#endif

typedef ascii cosm_SEMAPHORE_NAME[32];

typedef struct cosm_SEMAPHORE
{
  u32 state;
  cosm_SEMAPHORE_NAME name;
#if( defined( WINDOWS_SEMAPHORES ) )
  HANDLE os_sem;
#elif ( defined( POSIX_SEMAPHORES ) )
  sem_t * os_sem;
#elif ( defined( SYSV_SEMAPHORES ) )
  int os_sem;
#else
#error "Unknown semaphore types - see os_task.h"
#endif
} cosm_SEMAPHORE;

typedef struct cosm_DYNAMIC_LIB
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE os_lib;
#else
  void * os_lib;
#endif
} cosm_DYNAMIC_LIB;

/* Process Functions */

u64 CosmProcessID( void );
  /*
    Returns: The current process ID, 0 indicates failure.
  */

u8 CosmProcessPriority( u8 priority );
  /*
    Attempt to set the task priority from 1 to 255. If 0 is passed, then
    no change will be made and the current priority will be returned.
    1 is low priority, 255 is high priority. The procedure will remap the
    priority into values appropriate for the system. This procedure will never
    increase the priority beyond the default normal (255) priority for the OS.
    Returns: Process priority (mapped to 1-255 scale) after call is completed,
      0 indicates failure.
   */

s32 CosmProcessSpawn( u64 * process_id, const ascii * command,
  const ascii * arguments, ... );
  /*
    Spawn a new process running command. The command must be a full path in
    Cosm format to the binary. process_id is set to the process ID of the
    swawned process. The arguments MUST end with a NULL.
    e.g. CosmProcessSpawn( "cat", "file.txt", "file2.txt", NULL );
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void CosmProcessEnd( int status );
  /*
    End the process and all threads, return status as the exit code.
    All programs should always exit this way.
    Returns: Nothing.
  */

/* CPU Functions */

s32 CosmCPUCount( u32 * count );
  /*
    Sets count to the number of CPU's in the system. count will always
    be set to 1 or more even on failure, which indicates the system was
    not able to detect additional CPUs.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmCPULock( u64 process_id, u32 cpu );
  /*
    Lock the process, and any threads, to the CPU cpu. This function
    is primarily for scheduling and secure memory allocation.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmCPUUnlock( u64 process_id );
  /*
    Unlock the process from a set CPU. If you are using any secure
    memory, calling this function would be a very very bad idea.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* Thread Functions */

s32 CosmThreadBegin( u64 * thread_id, void (*start)(void *),
  void * arg, u32 stack_size );
  /*
    Begin a thread, make a stack, and call the function start.
    thread_id is set to the thread ID of the new thread.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

u64 CosmThreadID( void );
  /*
    Returns: The current thread ID, 0 indicates failure.
  */

u8 CosmThreadPriority( u8 priority );
  /*
    Attempt to set the thread priority from 1 to 255. If 0 is passed, then
    no change will be made and the current priority will be returned.
    1 is low priority, 255 is high priority. The procedure will remap the
    priority into values appropriate for the system. This procedure will
    never increase the priority beyond the default normal (255) priority for
    the OS.
    Returns: Thread priority (mapped to 1-255 scale) after call is completed,
      0 indicates failure.
  */

void CosmThreadEnd( void );
  /*
    End the calling thread.
    Returns: Nothing, thread's dead.
  */

/* Locks */

s32 CosmMutexInit( cosm_MUTEX * mutex );
  /*
    Create a mutex.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmMutexLock( cosm_MUTEX * mutex, u32 wait );
  /*
    Set an exclusive lock.
    If wait is COSM_MUTEX_WAIT then do not return until lock is available.
    If wait is COSM_MUTEX_NOWAIT, then return immediately.
    Locking a lock you already hold, or unlocking one you didn't hold are
    undefined on some platforms.
    It's possible under catastophic conditions that an OS error could be
    detected and the process terminated.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void CosmMutexUnlock( cosm_MUTEX * mutex );
  /*
    Unset an exclusive lock. Done in the same thread that locked.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void CosmMutexFree( cosm_MUTEX * mutex );
  /*
    Free the mutex and related system resources. Do not call this unless
    you are completely done with the mutex, or very bad things will happen.
    Returns: nothing.
  */

/* Semaphores */

s32 CosmSemaphoreInit( cosm_SEMAPHORE * sem, u32 initial_count );
  /*
    Create a semaphore with initial value of initial_count.
    Note: Picking a name is not allowed due to platform specific naming
    conventions.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmSemaphoreOpen( cosm_SEMAPHORE * sem, cosm_SEMAPHORE_NAME * name );
  /*
    Open a semaphore initialized in another process. You must later
    close (not free) any semaphore acquired in this way.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void CosmSemaphoreClose( cosm_SEMAPHORE * sem );
  /*
    Close a semaphore that has been opened.
    Returns: nothing.
  */

s32 CosmSemaphoreDown( cosm_SEMAPHORE * sem, u32 wait );
  /*
    Decrease the count of the semaphore by 1.
    If wait is COSM_SEMAPHORE_WAIT then do not return until available.
    If wait is COSM_SEMAPHORE_NOWAIT, then return immediately.
    It's possible under catastophic conditions that an OS error could be
    detected and the process terminated.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmSemaphoreUp( cosm_SEMAPHORE * sem );
  /*
    Release a semaphore and add 1 to it's value.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void CosmSemaphoreFree( cosm_SEMAPHORE * sem );
  /*
    Free the semaphore and related system resources. This can only be done by
    the process that init'd the semaphore, not one that opened it. This should
    not be done until all procsses are finished with the semaphore, as any
    further use may cause errors or hung processes depending on the OS.
    Returns: nothing.
  */

/* Sleep */

void CosmSleep( u32 millisec );
  /*
    Sleep for millisec milliseconds.
    Returns: nothing
  */

void CosmYield( void );
  /*
    Yield to another thread/process that wants to run.
    Returns: nothing
  */

/* Signals */

s32 CosmSignal( u64 process_id, u32 signal_type );
  /*
    Sends signal to process.
    Valid signals are:
      COSM_SIGNAL_PING  Test for Process Existance
      COSM_SIGNAL_INT   Save/Checkpoint or other message.
      COSM_SIGNAL_TERM  Save and Terminate
      COSM_SIGNAL_KILL  Terminate with extreme prejudice
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmSignalRegister( u32 signal_type, void (*handler)(int) );
  /*
    Register handler as the handler for the signal.signal_type must be either
    COSM_SIGNAL_INT or COSM_SIGNAL_TERM. See CosmSignal for sending
    signals. Once the handler is triggered the OS will reset the handler, so
    if you wish to reuse a signal, you must reregister it in the handler.
    Inside of your signal handler you must be very careful as any thread
    may end up running the handler at any time. Make your handler as short
    as possible.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* Dynamic libraries */

s32 CosmDynamicLibLoad( cosm_DYNAMIC_LIB * dylib, const ascii * filename );
  /*
    Load the library found at filename. Your libraries should NOT use
    or depend on on an auto-initialization function (e.g. _init or DllMain)
    due to their limitations. Export an explicit initialization function
    for the user to call if neccesary.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

void * CosmDynamicLibGet( cosm_DYNAMIC_LIB * dylib, const ascii * symbol );
  /*
    Get the address of the symbol in the library. The symbol should be the
    plain name without underscores or other system specific notation.
    Returns: Address of the symbol or NULL on failure.
  */

void CosmDynamicLibFree( cosm_DYNAMIC_LIB * dylib );
  /*
    Unload the library.
    Returns: nothing.
  */

/* Time */

s32 CosmSystemClock( cosmtime * clock );
  /*
    Sets the value of clock from the system clock.
    cosmtime is a signed number (s128) of seconds in 64b.64b fixed point
    format based on the time 0 = 00:00:00 UTC, Jan 1, 2000 AD.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

/* Low level helper functions */

u8 Cosm_PriorityToCosm( int pri );
  /*
    Translate native priority range to Cosm range.
    Returns: Priority in Cosm range.
  */

int Cosm_PriorityToOS( u8 pri );
  /*
    Translate Cosm priority range to system range.
    Returns: Priority in native system terms.
  */

/* testing */

void Cosm_ThreadTestSrc( void * arg );
  /*
    This thread will produce data and put it into the buffer starting with
    0 and counting to 255.
  */

void Cosm_ThreadTestDest( void * arg );
  /*
    This thread will consume data and take it out of the buffer,
    it will expect numbers starting with 0 and counting to 255.
  */

s32 Cosm_TestOSTask( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
