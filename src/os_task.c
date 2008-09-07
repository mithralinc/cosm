/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - CPU/OS Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2007 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

/* CPU/OS Layer - CPU and OS specific code is allowed */

#include "cosm/os_task.h"
#include "cosm/os_file.h"
#include "cosm/os_io.h"
#include "cosm/os_mem.h"
#include "cosm/os_net.h"

#include <errno.h>
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#include <process.h>
#include <time.h>
u32 win32_signals_init = 0;
HANDLE win32_sigint_handle = NULL;
HANDLE win32_sigterm_handle = NULL;
void Cosm_SignalWaitThread( void * arg );
#elif ( OS_TYPE == OS_SUNOS )
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <lwp/lwp.h>
#include <lwp/lwpmachdep.h>
#include <lwp/stackdep.h>
#include <dlfcn.h>
#elif ( OS_TYPE == OS_MACOSX )
#include <sched.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <dlfcn.h>
#else /* OS */
#if ( OS_TYPE == OS_NETBSD )
#include <sys/sched.h>
#else
#include <sched.h>
#endif
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <dlfcn.h>
#endif /* OS */

#if ( OS_TYPE == OS_FREEBSD )
#include <sys/sysctl.h>
#endif

#if ( OS_TYPE == OS_OPENBSD )
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if ( ( OS_TYPE == OS_IRIX ) || ( OS_TYPE == OS_IRIX64 ) )
#include <sys/sysmp.h>
#endif

#if ( OS_TYPE == OS_SOLARIS )
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif

/* Setup the size of Process ID */
#if ( 0 )
#define COSM_PROCESSID_64
#else
#undef COSM_PROCESSID_64
#endif

/* Solaris lacks PRIO_MAX and PRIO_MIN. */
#if ( OS_TYPE == OS_SOLARIS )
#define PRIO_MAX  20
#define PRIO_MIN  -20
#endif

/* QNX lacks PRIO_MAX and PRIO_MIN. */
#if ( OS_TYPE == OS_QNX )
#define PRIO_MAX 63
#define PRIO_MIN 1
#endif

u64 CosmProcessID( void )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  return (u64) GetCurrentProcessId();
#else /* unix... */
  return (u64) getpid();
#endif /* OS */
}

u8 CosmProcessPriority( u8 priority )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* same code as for threads */
  return CosmThreadPriority( priority );
#else /* UNIX */
  s32 get_pri;

  if ( priority != 0 )
  {
    if ( setpriority( PRIO_PROCESS, 0, Cosm_PriorityToOS( priority ) ) != 0 )
    {
      return 0;
    }
  }

  errno = 0;
  get_pri = getpriority( PRIO_PROCESS, 0 );

  if ( errno == 0 )
  {
    return Cosm_PriorityToCosm( get_pri );
  }
  else
  {
    return 0;
  }
#endif
}

s32 CosmProcessSpawn( u64 * process_id, const ascii * command,
  const ascii * arguments, ... )
{
  const ascii * arg_array[1024];
  cosm_FILENAME filename;
  va_list args;
  u32 i;
  int new_pid;

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  ascii command_line[1024];
  STARTUPINFO startup_info;
  PROCESS_INFORMATION process_info;
  u32 j;
  u32 length;
  u32 total;
  u32 done;
#endif

  *process_id = (u64) 0;

  if ( Cosm_FileNativePath( filename, command ) == COSM_FAIL )
  {
    return COSM_FAIL;
  }

  arg_array[0] = (const ascii *) filename;
  arg_array[1] = arguments;

  va_start( args, arguments );
  i = 1;
  while ( ( arg_array[i] != NULL ) && ( i < 1023 ) )
  {
    arg_array[++i] = va_arg( args, const ascii * );
  }
  arg_array[1023] = NULL;
  va_end( args );

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* put together a command line, win32 doesn't have a working spawnv() */
  total = 0;
  j = 0;
  done = 0;
  while ( ( j < i ) && ( done == 0 ) )
  {
    length = CosmStrBytes( arg_array[j] );
    if ( length > ( 1022 - total ) )
    {
      done = 1;
    }
    else
    {
      if ( j > 0 )
      {
        command_line[total++] = ' ';
        CosmStrCopy( &command_line[total], arg_array[j], 1024 - total );
        total += length;
      }
      else
      {
        command_line[total++] = '"';
        CosmStrCopy( &command_line[total], arg_array[j], 1024 - total );
        total += length;
        command_line[total++] = '"';
      }
      j++;
    }
  }
  CosmMemSet( &startup_info, sizeof( STARTUPINFO ), (u8) 0 );
  startup_info.cb = sizeof( STARTUPINFO );
  if ( CreateProcess( NULL, (char *) command_line,
    NULL, NULL, 0, 0, NULL, NULL, &startup_info, &process_info ) == 0 )
  {
    return COSM_FAIL;
  }
  else
  {
    /* process will not go away until all the handles are closed */
    CloseHandle( process_info.hProcess );
    CloseHandle( process_info.hThread );
    new_pid = process_info.dwProcessId;
  }
#else /* unix... */
#if ( defined( SIGCHLD ) )
  /* SIGCHLD has to be ignored or zombies remain and confuse kill() */
  signal( SIGCHLD, SIG_IGN );
#endif
  new_pid = fork();
  if ( new_pid == -1 ) /* check for failure */
  {
    return COSM_FAIL;
  }
  if ( new_pid == 0 ) /* are we in the child or parent */
  {
    /* in child */
    if ( execvp( (char *) arg_array[0], (char * *) arg_array ) == -1 )
    {
      /* since we are in the child, this is an unrecoverable error. */
      exit( 0 );
    }
  }
#endif

  *process_id = (u64) new_pid;

  return COSM_PASS;
}

void CosmProcessEnd( int status )
{
  /* do any Cosm specific cleanup */

  exit( status );
}

s32 CosmCPUCount( u32 * count )
{
  /*
    There must be one CPU, but we must find out how many as a normal user
    and without any 'tricks' like reading /proc or /dev.
  */
#ifdef _SC_NPROCESSORS_ONLN
  /* Linux 2.4+, Solaris, Tru64 */
  *count = sysconf( _SC_NPROCESSORS_ONLN );
  if ( *count < 1 )
  {
    *count = 1;
    return COSM_FAIL;
  }
#elif ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  SYSTEM_INFO info;

  GetSystemInfo( &info );
  *count = info.dwNumberOfProcessors;
#elif ( ( OS_TYPE == OS_MACOSX ) || ( OS_TYPE == OS_FREEBSD ) \
  || ( OS_TYPE == OS_OPENBSD ) )
  int mib[2], cpu;
  size_t len;

  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  len = sizeof( cpu );
  if ( sysctl( mib, 2, &cpu, &len, NULL, 0 ) != -1 )
  {
    *count = cpu;
  }
  else
  {
    *count = 1;
    return COSM_FAIL;
  }
#elif ( ( OS_TYPE == OS_IRIX ) || ( OS_TYPE == OS_IRIX64 ) )
  *count = sysmp( MP_NPROCS );
#elif ( OS_TYPE == OS_SUNOS )
  /* SunOS does have multiple CPU capabilities, but no sysconf()
     interface to access it. */
  *count = 1;
  return COSM_FAIL;
#else /* OS = other */
  *count = 1;
  return COSM_FAIL;
#endif /* OS */

  return COSM_PASS;
}

s32 CosmCPULock( u64 process_id, u32 cpu )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE process;
  DWORD proc_mask, sys_mask;

  if ( ( process = OpenProcess( PROCESS_QUERY_INFORMATION
    | PROCESS_SET_INFORMATION, 0, (u32) process_id ) ) == NULL )
  {
    return COSM_FAIL;
  }
  else
  {
    if ( GetProcessAffinityMask( process, &proc_mask, &sys_mask ) == 0 )
    {
      CloseHandle( process );
      return COSM_FAIL;
    }
    /* catch old versions where SetProcessAffinityMask() always fails */
    if ( ( sys_mask != 1 ) || ( cpu != 0 ) )
    {
      proc_mask = 1 << cpu;
      if ( SetProcessAffinityMask( process, proc_mask ) == 0 )
      {
        CloseHandle( process );
        return COSM_FAIL;
      }
    }
    CloseHandle( process );
    return COSM_PASS;
  }
#elif ( ( OS_TYPE == OS_LINUX ) && ( defined( sched_setaffinity ) ) )
  u32 mask;

  if ( process_id == 0 )
  {
    return COSM_FAIL;
  }
  mask = 1 << cpu;
#if ( defined( COSM_PROCESSID_64 ) )
  if ( sched_setaffinity( (pid_t) process_id, sizeof( mask ), &mask ) == -1 )
#else
  if ( sched_setaffinity( (pid_t) (u32) process_id,
    sizeof( mask ), &mask ) == -1 )
#endif
  {
    return COSM_FAIL;
  }
  return COSM_PASS;
#elif ( OS_TYPE == OS_SOLARIS )
  int result;

  result = processor_bind( P_PID, (pid_t) (u32) process_id,
    (processorid_t) cpu, NULL );
  if ( result == 0 )
  {
    return COSM_PASS;
  }
  return COSM_FAIL;
#else /* OS */
  return COSM_FAIL;
#endif
}

s32 CosmCPUUnlock( u64 process_id )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE process;
  DWORD proc_mask, sys_mask;

  if ( ( process = OpenProcess( PROCESS_SET_INFORMATION
    | PROCESS_QUERY_INFORMATION, 0, (u32) process_id ) ) == NULL )
  {
    return COSM_FAIL;
  }
  else
  {
    if ( GetProcessAffinityMask( process, &proc_mask, &sys_mask ) == 0 )
    {
      CloseHandle( process );
      return COSM_FAIL;
    }
    /* catch old versions where SetProcessAffinityMask() always fails */
    if ( sys_mask != 1 )
    {
      if ( SetProcessAffinityMask( process, sys_mask ) == 0 )
      {
        CloseHandle( process );
        return COSM_FAIL;
      }
    }
    CloseHandle( process );
    return COSM_PASS;
  }
#elif ( ( OS_TYPE == OS_LINUX ) && ( defined( sched_setaffinity ) ) )
  u32 mask;

  if ( process_id == 0 )
  {
    return COSM_FAIL;
  }

  mask = 0xFFFFFFFF;
#if ( defined( COSM_PROCESSID_64 ) )
  if ( sched_setaffinity( (pid_t) process_id, sizeof( mask ), &mask ) == -1 )
#else
  if ( sched_setaffinity( (pid_t) (u32) process_id,
    sizeof( mask ), &mask ) == -1 )
#endif
  {
    return COSM_FAIL;
  }
  return COSM_PASS;
#elif ( OS_TYPE == OS_SOLARIS )
  int result;
  result = processor_bind( P_PID, (pid_t) (u32) process_id,
    PBIND_NONE, NULL );
  if ( result == 0 )
  {
    return COSM_PASS;
  }
  return COSM_FAIL;
#else /* OS */
  return COSM_FAIL;
#endif
}

s32 CosmThreadBegin( u64 * thread_id, void (*start)(void *),
  void * arg, u32 stack_size )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  u32 t_id;
  HANDLE thandle;

  thandle = (HANDLE) _beginthreadex( NULL, stack_size,
    (unsigned int (__stdcall *)(void *)) start, arg, 0, &t_id );
  if ( thandle == 0 )
  {
    *thread_id = 0;
    return COSM_FAIL;
  }
  else
  {
    /* thread will not go away until all the handles are closed */
    CloseHandle( thandle );
    *thread_id = (u64) t_id;
    return COSM_PASS;
  }
#elif ( OS_TYPE == OS_SUNOS )
  u64 result;
  thread_t tid;
  stkalign_t *stack;
  int ret;

  *thread_id = (u64) 0;

  /* !!! use a thread stack cache? [init] */
  /* minimum stack size */
  if ( stack_size < ( MINSIGSTKSZ ) )
  {
    stack_size = MINSIGSTKSZ;
  }

  /* Allocate the stack */
  stack = (stkalign_t *) malloc( sizeof(stkalign_t) * stack_size );
  if ( stack == NULL )
  {
    return COSM_FAIL;
  }

  /* Create the critter */
  ret = lwp_create( &tid, (void * (*)( void * )) start, MINPRIO, 0,
    stack, 1, (int) arg, (int) 0, (int) 0, (int) 0);
  if ( ret != 0 )
  {
    return COSM_FAIL;
  }
  else
  {
    /* SunOS is 32bit only. */
    result.hi = (u32) tid.thread_id;
    result.lo = (u32) tid.thread_key;
  }
  *thread_id = result;
  return COSM_PASS;
#else /* POSIX Threads? */
  u64 result;
  pthread_t thread;
  pthread_attr_t attr;

  *thread_id = (u64) 0;

  pthread_attr_init( &attr );

#if ( ( OS_TYPE == OS_IRIX ) || ( OS_TYPE == OS_IRIX64 ) \
  || ( OS_TYPE == OS_MACOSX ) )
  pthread_attr_setstacksize( &attr, (size_t) stack_size );
#elif ( OS_TYPE == OS_SOLARIS )
  /* system-wide contention */
  pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
#endif

  result = (u64) pthread_create( &thread, &attr,
    (void * (*)( void * )) start, arg );
  pthread_attr_destroy( &attr );

  if ( result !=  0 )
  {
    return COSM_FAIL;
  }
  else
  {
#ifdef CPU_64BIT
    *thread_id = (u64) thread;
#else
    *thread_id = (u32) thread;
#endif
    return COSM_PASS;
  }
#endif
}

u64 CosmThreadID( void )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  return (u64) GetCurrentThreadId();
#elif ( OS_TYPE == OS_SUNOS )
  u64 thread_id;
  thread_t tid;

  if ( lwp_self( &tid ) != 0 )
  {
    return (u64) 0;
  }
  /* SunOS is 32bit only. */
  thread_id->hi = (u32) tid.thread_id;
  thread_id->lo = (u32) tid.thread_key;
  return thread_id;
#else /* OS */
#ifdef CPU_64BIT
  return (u64) pthread_self();
#else
  return (u32) pthread_self();
#endif
#endif
}

u8 CosmThreadPriority( u8 priority )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  s32 goal, process, thread;
  if ( priority != 0 )
  {
    goal = Cosm_PriorityToOS( priority );
    /*
      this table uses the priority scale for foreground processes, using the
      values for background proccesses caused major problems
    */
    switch ( goal )
    {
      case 1:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_IDLE;
        break;
      case 2:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_LOWEST;
        break;
      case 3:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_BELOW_NORMAL;
        break;
      case 4:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_NORMAL;
        break;
      case 5:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
      case 6:
        process = IDLE_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_HIGHEST;
        break;
      case 7:
        process = NORMAL_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_LOWEST;
        break;
      case 8:
        process = NORMAL_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_BELOW_NORMAL;
        break;
      default: /* 9, default */
        process = NORMAL_PRIORITY_CLASS;
        thread = THREAD_PRIORITY_NORMAL;
        break;
    }
    if ( ( SetPriorityClass( GetCurrentProcess(), process ) == 0 ) ||
      ( SetThreadPriority( GetCurrentThread(), thread ) == 0 ) )
    {
      return 0;
    }
  }

  if ( ( ( process = GetPriorityClass( GetCurrentProcess() ) ) == 0 ) ||
    ( ( thread = GetThreadPriority( GetCurrentThread() ) ) ==
    THREAD_PRIORITY_ERROR_RETURN ) )
  {
    return 0;
  }

  if ( process == IDLE_PRIORITY_CLASS )
  {
    switch ( thread )
    {
      case THREAD_PRIORITY_IDLE:
        goal = 1;
        break;
      case THREAD_PRIORITY_LOWEST:
        goal = 2;
        break;
      case THREAD_PRIORITY_BELOW_NORMAL:
        goal = 3;
        break;
      case THREAD_PRIORITY_NORMAL:
        goal = 4;
        break;
      case THREAD_PRIORITY_ABOVE_NORMAL:
        goal = 5;
        break;
      case THREAD_PRIORITY_HIGHEST:
        goal = 6;
        break;
      default:
        goal = 9;
        break;
    }
  }
  else if ( process == NORMAL_PRIORITY_CLASS )
  {
    switch ( thread )
    {
      case THREAD_PRIORITY_IDLE:
        goal = 1;
        break;
      case THREAD_PRIORITY_LOWEST:
        goal = 7;
        break;
      case THREAD_PRIORITY_BELOW_NORMAL:
        goal = 8;
        break;
      default: /* THREAD_PRIORITY_NORMAL, default */
        goal = 9;
        break;
    }
  }
  else
  {
    goal = 9;
  }

  return Cosm_PriorityToCosm( (u8) goal );
#elif ( OS_TYPE == OS_SUNOS )
  int result;

  result = lwp_setpri( SELF, priority );
  if ( result != 0 )
  {
    /* !!! call pod_setmaxpri(255) maybe? [init] */
    return 0;
  }
  return priority;
#else /* OS */
  pthread_attr_t attr;
  struct sched_param param;
  u8 pri_set;
  u8 pri_result;

  pri_set = Cosm_PriorityToCosm( priority );
  pri_result = 0;

  if ( priority != 0 )
  {
    pthread_attr_init( &attr );

    pthread_attr_getschedparam( &attr, &param );
    if ( param.sched_priority != pri_set )
    {
      param.sched_priority = pri_set;
      pri_result = pthread_attr_setschedparam( &attr, &param );
    }
    pthread_attr_destroy( &attr );
  }
  return Cosm_PriorityToCosm( pri_result );
#endif
}

void CosmThreadEnd( void )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  _endthreadex( 0 );
#elif ( OS_TYPE == OS_SUNOS )
  lwp_destroy( SELF );
#else /* OS */
  pthread_exit( NULL );
#endif
}

s32 CosmMutexInit( cosm_MUTEX * mutex )
{
#if ( defined( _POSIX_THREADS ) )
#if ( ( OS_TYPE == OS_LINUX ) || ( OS_TYPE == OS_NETBSD ) \
  || ( OS_TYPE == OS_MACOSX ) )
  pthread_mutex_t tmp_mutex = PTHREAD_MUTEX_INITIALIZER;
#else /* standard POSIX case */
  pthread_mutexattr_t attr;
#endif /* types of pthread mutex initialization */
#endif

  if ( ( mutex == NULL ) || ( mutex->state == COSM_MUTEX_STATE_INIT ) )
  {
    /* no double-init */
    return COSM_FAIL;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( ( mutex->os_mutex = CreateMutex( NULL, FALSE, NULL ) ) == NULL )
  {
    return COSM_FAIL;
  }
#elif ( defined( _POSIX_THREADS ) )
  mutex->os_mutex = tmp_mutex;
/* alternative POSIX method
#else
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_NORMAL );
  pthread_mutex_init( &mutex->os_mutex, &attr );
  pthread_mutexattr_destroy( &attr );
#endif
*/
#elif ( defined( SEM_VALUE_MAX ) ) /* no posix threads */
  if ( sem_init( &mutex->os_mutex, 0, 1 ) != 0 )
  {
    return COSM_FAIL; 
  }
#else
#error "no mutex? check CosmMutexInit()"
#endif /* OS */

  mutex->state = COSM_MUTEX_STATE_INIT;
  return COSM_PASS;
}

s32 CosmMutexLock( cosm_MUTEX * mutex, u32 wait )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  DWORD result;
#endif

  if ( ( mutex == NULL ) || ( mutex->state != COSM_MUTEX_STATE_INIT ) )
  {
    return COSM_FAIL;
  }

  if ( wait == COSM_MUTEX_WAIT )
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    result = WaitForSingleObject( mutex->os_mutex, INFINITE );
    if ( result == WAIT_OBJECT_0 )
    {
      return COSM_PASS;
    }
    else if ( result == WAIT_ABANDONED )
    {
      /* Documented as very very bad */
      CosmPrint( "Exit due to thread death in a Mutex\n" );
      CosmProcessEnd( -1 );
    }
    else
    {
      /* non-fatal */
      return COSM_FAIL;
    }
#elif ( defined( _POSIX_THREADS ) )
    if ( pthread_mutex_lock( &mutex->os_mutex ) != 0 )
    {
      return COSM_FAIL;
    }
#elif ( defined( SEM_VALUE_MAX ) )
    if ( sem_wait( &mutex->os_mutex ) != 0 )
    {
      return COSM_FAIL;
    }
#else
#error "threads but no mutexes? check CosmMutexLock()"
#endif /* OS */
  }
  else if ( wait == COSM_MUTEX_NOWAIT )
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    result = WaitForSingleObject( mutex->os_mutex, 0 );
    if ( result == WAIT_OBJECT_0 )
    {
      return COSM_PASS;
    }
    else if ( result == WAIT_ABANDONED )
    {
      /* Documented as very very bad */
      CosmPrint( "Exit due to thread death in a Mutex\n" );
      CosmProcessEnd( -1 );
    }
    else
    {
      /* proabably just a timeout, non-fatal */
      return COSM_FAIL;
    }
#elif ( defined( _POSIX_THREADS ) )
    if ( pthread_mutex_trylock( &mutex->os_mutex ) != 0 )
    {
      return COSM_FAIL;
    }
#elif ( defined( SEM_VALUE_MAX ) )
    if ( sem_trywait( &mutex->os_mutex ) != 0 )
    {
      return COSM_FAIL;
    }
#else /* no OS mutex support, but threads */
  /* should never be allowed to happen */
#error "threads but no mutexes? check CosmMutexLock()"
#endif /* OS */
  }
  else /* wait is invalid */
  {
    return COSM_FAIL;
  }

  return COSM_PASS;
}

void CosmMutexUnlock( cosm_MUTEX * mutex )
{
  if ( mutex->state != COSM_MUTEX_STATE_INIT )
  {
    return;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  ReleaseMutex( mutex->os_mutex );
#elif ( defined( _POSIX_THREADS ) )
  pthread_mutex_unlock( &mutex->os_mutex );
#elif ( defined( SEM_VALUE_MAX ) )
  sem_post( &mutex->os_mutex );
#else
#error "threads but no mutexes? check CosmMutexUnlock()"
#endif /* OS */
}

void CosmMutexFree( cosm_MUTEX * mutex )
{
  if ( mutex->state != COSM_MUTEX_STATE_INIT )
  {
    return;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  CloseHandle( mutex->os_mutex );
#elif ( defined( _POSIX_THREADS ) )
  pthread_mutex_destroy( &mutex->os_mutex );
#elif ( defined( SEM_VALUE_MAX ) )
  sem_destroy( &mutex->os_mutex );
#else /* no OS mutex support, but threads */
  /* should never be allowed to happen */
#error "threads but no mutexes? check CosmMutexFree()"
#endif /* OS */

  mutex->state = 0;
}

s32 CosmSemaphoreInit( cosm_SEMAPHORE * sem, cosm_SEMAPHORE_NAME * name,
  u32 initial_count )
{
  u32 unique_found = 0;
  u64 key;
  
  if ( ( NULL == sem ) || ( NULL == name )
    || ( sem->state == COSM_SEMAPHORE_STATE_INIT )
    || ( sem->state == COSM_SEMAPHORE_STATE_OPEN ) )
  {
    return COSM_FAIL;
  }
  
  key = (unsigned long) sem;
  key = CosmProcessID() ^ ( ( key << 32 ) & ( key >> 32 ) ); 
  do
  {    
#if ( defined( WINDOWS_SEMAPHORES ) )
    CosmPrintStr( sem->name, sizeof( cosm_SEMAPHORE_NAME ),
      "Global\\sem%Y", key );

    if ( NULL == ( sem->os_sem = CreateSemaphore( NULL, initial_count,
      0x7FFFFFFF, sem->name ) ) )
    {
      return COSM_FAIL;
    }

    if ( ERROR_ALREADY_EXISTS == GetLastError() )
    {
      /* already in use */
      CloseHandle( sem->os_sem );
    }
    else
    {
      unique_found = 1;
    }
#elif ( defined( POSIX_SEMAPHORES ) )
    CosmPrintStr( sem->name, sizeof( cosm_SEMAPHORE_NAME ), "/sem%Y", key );

    if ( (void *) SEM_FAILED == ( sem->os_sem = sem_open( sem->name,
      O_CREAT | O_EXCL, S_IRWXU, initial_count ) ) )
    {
      if ( EEXIST != errno )
      {
        return COSM_FAIL;
      }
      /* otherwise keep trying */
    }
    else
    {
      unique_found = 1;
    }
#elif ( defined( SYSV_SEMAPHORES ) )
    CosmPrintStr( sem->name, sizeof( cosm_SEMAPHORE_NAME ), "sem%Y", key );

    if ( -1 == ( sem->os_sem = semget( key, 1,
      IPC_CREAT | IPC_EXCL | 0600 ) ) )
    {
      if ( EEXIST != errno )
      {
        return COSM_FAIL;
      }
      /* ok to keep trying */
    }
    else
    {
      if ( -1 == semctl( sem->os_sem, 0, SETVAL, (int) initial_count ) )
      {
        /* cleanup semaphore and fail */
        semctl( sem->os_sem, 0, IPC_RMID );
        return COSM_FAIL;
      }
      unique_found = 1;
    }
#else
#error "Incomplete CosmSemaphoreInit - see os_task.c"
#endif /* semaphore type */
    key++;
  } while ( 0 == unique_found );
  
  CosmStrCopy( (utf8 *) name, sem->name, sizeof( cosm_SEMAPHORE_NAME ) );
  sem->state = COSM_SEMAPHORE_STATE_INIT;
  return COSM_PASS;
}

s32 CosmSemaphoreOpen( cosm_SEMAPHORE * sem, cosm_SEMAPHORE_NAME * name )
{
#if ( defined( SYSV_SEMAPHORES ) )
  u64 key;
#endif

  if ( ( NULL == sem ) || ( NULL == name )
    || ( COSM_SEMAPHORE_STATE_INIT == sem->state )
    || ( COSM_SEMAPHORE_STATE_OPEN == sem->state ) )
  {
    return COSM_FAIL;
  }

#if ( defined( WINDOWS_SEMAPHORES ) )
  if ( NULL == ( sem->os_sem = OpenSemaphore(
    SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, (char *) name ) ) )
  {
    return COSM_FAIL;
  }
#elif ( defined( POSIX_SEMAPHORES ) )
  if ( (void *) SEM_FAILED == ( sem->os_sem = sem_open( (char *) name, 0 ) ) )
  {
    return COSM_FAIL;
  }
#elif ( defined( SYSV_SEMAPHORES ) )
  if ( ( COSM_FAIL == CosmU64Str( &key, NULL, &name[0][3], 16 ) )
    || ( -1 == ( sem->os_sem = semget( key, 0, 0 ) ) ) )
  {
CosmPrint( "\n sem fail %i %i\n", key,  errno );
    return COSM_FAIL;
  }
#else
#error "Incomplete CosmSemaphoreOpen - see os_task.c"
#endif /* semaphore type */

  CosmStrCopy( sem->name, (utf8 *) name, sizeof( cosm_SEMAPHORE_NAME ) );
  sem->state = COSM_SEMAPHORE_STATE_OPEN;
  return COSM_PASS;
}

void CosmSemaphoreClose( cosm_SEMAPHORE * sem )
{
  if ( ( NULL == sem ) || ( COSM_SEMAPHORE_STATE_OPEN != sem->state ) )
  {
    return;
  }

#if ( defined( WINDOWS_SEMAPHORES ) )
  if ( 0 == CloseHandle( sem->os_sem ) )
  {
    return;
  }
#elif ( defined( POSIX_SEMAPHORES ) )
  if ( -1 == sem_close( sem->os_sem ) )
  {
    return;
  }
#elif ( defined( SYSV_SEMAPHORES ) )
  /* do nothing, only the creator should remove the semaphore */
#else
#error "Incomplete CosmSemaphoreClose - see os_task.c"
#endif /* semaphore type */

  CosmMemSet( sem, sizeof( cosm_SEMAPHORE ), 0 );
}

s32 CosmSemaphoreDown( cosm_SEMAPHORE * sem, u32 wait )
{
#if ( defined( WINDOWS_SEMAPHORES ) )
  DWORD result;
#endif
#if ( defined( SYSV_SEMAPHORES ) )
  struct sembuf command;
#endif

  if ( ( sem == NULL ) || ( ( sem->state != COSM_SEMAPHORE_STATE_INIT ) 
    && ( sem->state != COSM_SEMAPHORE_STATE_INIT ) ) )
  {
    return COSM_FAIL;
  }

  if ( wait == COSM_SEMAPHORE_WAIT )
  {
#if ( defined( WINDOWS_SEMAPHORES ) )
    result = WaitForSingleObject( sem->os_sem, INFINITE );
    if ( result == WAIT_OBJECT_0 )
    {
      return COSM_PASS;
    }
    else if ( result == WAIT_ABANDONED )
    {
      /* Documented as very very bad */
      CosmPrint( "Exit due to thread death in a semaphore\n" );
      CosmProcessEnd( -1 );
    }
    else
    {
      /* non-fatal */
      return COSM_FAIL;
    }
#elif ( defined( POSIX_SEMAPHORES ) )
    wait_on_sem:
    if ( sem_wait( sem->os_sem ) == -1 )
    {
      if ( EINTR == errno )
      {
        /* interupted by a signal, need to rewait */
        goto wait_on_sem;
      }
      /* everything else is a real error */
      return COSM_FAIL;
    }
#elif ( defined( SYSV_SEMAPHORES ) )
    command.sem_num = 0;
    command.sem_op = -1;
    command.sem_flg = 0;
    
    wait_on_sem:
    if ( -1 == semop( sem->os_sem, &command, 1 ) )
    {
      if ( EINTR == errno )
      {
        /* interupted by a signal, need to rewait */
        goto wait_on_sem;
      }
      /* everything else is a real error */
      return COSM_FAIL;
    }
#else
#error "Incomplete CosmSemaphoreDown - see os_task.c"
#endif /* semaphore type */
  }
  else if ( wait == COSM_SEMAPHORE_NOWAIT )
  {
#if ( defined( WINDOWS_SEMAPHORES ) )
    result = WaitForSingleObject( sem->os_sem, 0 );
    if ( result == WAIT_OBJECT_0 )
    {
      return COSM_PASS;
    }
    else if ( result == WAIT_ABANDONED )
    {
      /* Documented as very very bad */
      CosmPrint( "Exit due to thread death in a semaphore\n" );
      CosmProcessEnd( -1 );
    }
    else
    {
      /* proabably just a timeout, non-fatal */
      return COSM_FAIL;
    }
#elif ( defined( POSIX_SEMAPHORES ) )
    if ( -1 == sem_trywait( sem->os_sem ) )
    {
      /* failures and already locked are both failures */
      return COSM_FAIL;
    }
#elif ( defined( SYSV_SEMAPHORES ) )
    command.sem_num = 0;
    command.sem_op = -1;
    command.sem_flg = IPC_NOWAIT;

    if ( -1 == semop( sem->os_sem, &command, 1 ) )
    {
      /* failures and already locked are both failures */
      return COSM_FAIL;
    }
#else
#error "Incomplete CosmSemaphoreDown - see os_task.c"
#endif /* semaphore type */
  }
  else /* wait is invalid */
  {
    return COSM_FAIL;
  }

  return COSM_PASS;
}

s32 CosmSemaphoreUp( cosm_SEMAPHORE * sem )
{
#if ( defined( SYSV_SEMAPHORES ) )
  struct sembuf command;
#endif

  if ( ( sem == NULL ) || ( ( sem->state != COSM_SEMAPHORE_STATE_INIT ) 
    && ( sem->state != COSM_SEMAPHORE_STATE_INIT ) ) )
  {
    return COSM_FAIL;
  }

#if ( defined( WINDOWS_SEMAPHORES ) )
  if ( ReleaseSemaphore( sem->os_sem, 1, NULL ) == 0 )
  {
    return COSM_FAIL;
  }
#elif ( defined( POSIX_SEMAPHORES ) )
  if ( sem_post( sem->os_sem ) == -1 )
  {
    return COSM_FAIL;
  }
#elif ( defined( SYSV_SEMAPHORES ) )
  command.sem_num = 0;
  command.sem_op = 1;
  command.sem_flg = 0;
  
  if ( -1 == semop( sem->os_sem, &command, 1 ) )
  {
    return COSM_FAIL;
  }
#else
#error "Incomplete CosmSemaphoreUp - see os_task.c"
#endif /* semaphore type */

  return COSM_PASS;
}

void CosmSemaphoreFree( cosm_SEMAPHORE * sem )
{
  if ( ( sem == NULL ) || ( sem->state != COSM_SEMAPHORE_STATE_INIT ) )
  {
    return;
  }

#if ( defined( WINDOWS_SEMAPHORES ) )
  CloseHandle( sem->os_sem );
#elif ( defined( POSIX_SEMAPHORES ) )
  sem_close( sem->os_sem );
  sem_unlink( sem->name );
#elif ( defined( SYSV_SEMAPHORES ) )
  semctl( sem->os_sem, 0, IPC_RMID );
#else
#error "Incomplete CosmSemaphoreFree - see os_task.c"
#endif /* semaphore type */

  CosmMemSet( sem, sizeof( cosm_SEMAPHORE ), 0 );
}

void CosmSleep( u32 millisec )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  Sleep( millisec );
#else
  struct timespec delay, remainder;

  delay.tv_sec = ( millisec / 1000 );
  delay.tv_nsec = ( ( millisec % 1000 ) * 1000000 );

  while ( nanosleep( &delay, &remainder ) != 0 )
  {
    if ( errno == EINTR )
    {
      /* We awoke early and need to re-sleep */
      delay = remainder;
    }
    else
    {
      return;
    }
  }
#endif
}

void CosmYield( void )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  SwitchToThread();
#elif ( OS_TYPE == OS_SUNOS )
  /* This doesn't do what you think it does... */
  /* (this will only yeild to the same lwp priority group) */
  lwp_yield( SELF );
#else /* OS */
#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#else
  sleep( 0 );
#endif
#endif /* OS */
}

s32 CosmSignal( u64 process_id, u32 signal_type )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE process;
  u32 status;
  ascii name[128];
#endif

  if ( signal_type == COSM_SIGNAL_PING )
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    if ( ( process = OpenProcess( PROCESS_QUERY_INFORMATION, 0,
      (u32) process_id ) ) == NULL )
    {
      return COSM_FAIL;
    }
    else
    {
      if ( ( GetExitCodeProcess( process, (unsigned long *) &status ) == 0 )
        || ( status != STILL_ACTIVE ) )
      {
        CloseHandle( process );
        return COSM_FAIL;
      }
      CloseHandle( process );
      return COSM_PASS;
    }
#else /* OS */
#if ( defined( COSM_PROCESSID_64 ) )
    if ( kill( process_id, 0 ) == -1 )
#else
    if ( kill( (u32) process_id, 0 ) == -1 )
#endif
    {
      if ( errno == ESRCH )
      {
        return COSM_FAIL;
      }
    }
#endif /* OS */
  }
  else
  {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    if ( process_id == CosmProcessID() )
    {
      /* process is this one */
      switch ( signal_type )
      {
        case COSM_SIGNAL_INT:
        case COSM_SIGNAL_TERM:
          raise( signal_type );
          break;
        case COSM_SIGNAL_KILL:
          CosmProcessEnd( -1 );
          break;
        default:
          return COSM_FAIL;
          break;
      }
      return COSM_PASS;
    }

    /* open the correct Semaphore, and trigger it */
    if ( signal_type == COSM_SIGNAL_KILL )
    {
      /* slay the process, this is not good for the system */
      if ( ( process = OpenProcess( PROCESS_ALL_ACCESS, 0,
        (u32) process_id ) ) == NULL )
      {
        return COSM_FAIL;
      }
      if ( TerminateProcess( process, 1 ) == 0 )
      {
        CloseHandle( process );
        return COSM_FAIL;
      }
      else
      {
        CloseHandle( process );
        return COSM_PASS;
      }
    }

    switch ( signal_type )
    {
      case COSM_SIGNAL_INT:
        CosmPrintStr( name, (u64) 128, "%016Y-SIGINT", process_id );
        break;
      case COSM_SIGNAL_TERM:
        CosmPrintStr( name, (u64) 128, "%016Y-SIGTERM", process_id );
        break;
      default:
        return COSM_FAIL;
    }
    if ( ( process = OpenSemaphore( SEMAPHORE_ALL_ACCESS, 0,
      (const char *) name ) ) == NULL )
    {
      return COSM_FAIL;
    }
    if ( ReleaseSemaphore( process, 1, NULL ) == 0 )
    {
      CloseHandle( process );
      return COSM_FAIL;
    }
    else
    {
      CloseHandle( process );
      return COSM_PASS;
    }
#else /* OS */
    if ( ( signal_type != COSM_SIGNAL_INT )
      && ( signal_type != COSM_SIGNAL_TERM )
      && ( signal_type != COSM_SIGNAL_KILL ) )
    {
      return COSM_FAIL;
    }
#if ( defined( COSM_PROCESSID_64 ) )
    if ( kill( (pid_t) process_id, (int) signal_type ) == -1 )
#else
    if ( kill( (pid_t) (u32) process_id, (int) signal_type ) == -1 )
#endif
    {
      return COSM_FAIL;
    }
#endif /* OS */
  }
  return COSM_PASS;
}

s32 CosmSignalRegister( u32 signal_type, void (*handler)(int) )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  ascii name[128];
  u64 pid;
  u64 thread_id;
  if ( win32_signals_init == 0 )
  {
    /* setup the Semaphores in unsignaled state */
    pid = CosmProcessID();
    if ( ( pid == 0x0000000000000000LL ) )
    {
      return COSM_FAIL;
    }
    CosmPrintStr( name, (u64) 128, "%016Y-SIGINT",
      CosmProcessID() );
    win32_sigint_handle = CreateSemaphore( NULL, 0, 1, (const char *) name );
    CosmPrintStr( name, (u64) 128, "%016Y-SIGTERM",
      CosmProcessID() );
    win32_sigterm_handle = CreateSemaphore( NULL, 0, 1, (const char *) name );
    if ( ( win32_sigint_handle == NULL ) || ( win32_sigint_handle == NULL ) )
    {
      return COSM_FAIL;
    }
    /* spawn a thread to go wait for a signals */
    if ( CosmThreadBegin( &thread_id, Cosm_SignalWaitThread, NULL, 4096 )
      != COSM_PASS )
    {
      return COSM_FAIL;
    }
    /* mark init as done */
    win32_signals_init = 1;
  }
#endif /* WIN32 */

  /* register the signal catchers */
  if ( ( signal_type == COSM_SIGNAL_INT )
    || ( signal_type == COSM_SIGNAL_TERM ) )
  {
    if ( signal( signal_type, handler ) != SIG_ERR )
    {
      return COSM_PASS;
    }
  }

  return COSM_FAIL;
}

/* Dynamic libraries */

s32 CosmDynamicLibLoad( cosm_DYNAMIC_LIB * dylib, const ascii * filename )
{
  cosm_FILENAME native_filename;
  void * result;

  if ( ( NULL == dylib ) || ( NULL == filename ) )
  {
    return COSM_FAIL;
  }

  /* translate filename */
  if ( Cosm_FileNativePath( native_filename, filename ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  result = LoadLibraryEx( native_filename, NULL, 0 );
#else /* OS */
  result = dlopen( native_filename, RTLD_LAZY | RTLD_GLOBAL );
#endif

  if ( result == NULL )
  {
    return COSM_FAIL;
  }
  else
  {
    dylib->os_lib = result;
    return COSM_PASS;
  }
}

void * CosmDynamicLibGet( cosm_DYNAMIC_LIB * dylib, const ascii * symbol )
{
  void * addr;
#if ( ( OS_TYPE != OS_WIN32 ) && ( OS_TYPE != OS_WIN64 ) )
  ascii with_underscore[1024];
#endif

  if ( ( NULL == dylib ) || ( NULL == symbol )
    || ( NULL == dylib->os_lib ) ) 
  {
    return NULL;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  addr = GetProcAddress( dylib->os_lib, symbol );
#else /* OS */
  addr = dlsym( dylib->os_lib, symbol );

  /* we may need an underscore in some UNIXs, so check on failure */
  if ( addr == NULL )
  {
    with_underscore[0] = '_';
    if ( CosmStrCopy( &with_underscore[1], symbol, 1023 ) == COSM_PASS )
    {
      addr = dlsym( dylib->os_lib, with_underscore );
    }
  }
#endif

  return addr;
}

void CosmDynamicLibFree( cosm_DYNAMIC_LIB * dylib )
{
  if ( ( NULL == dylib ) || ( NULL == dylib->os_lib ) )
  {
    return;
  }
  
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  FreeLibrary( dylib->os_lib );
#else /* OS */
  dlclose( dylib->os_lib );
#endif
}

/* Time */

s32 CosmSystemClock( cosmtime * local_time )
{
  s128 os_local_time, unix_epoc;

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  time_t utctime;
  SYSTEMTIME systime;
#else
  struct timeval time_value;
#endif

  if ( local_time == NULL )
  {
    return COSM_FAIL;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  do
  {
    time( &utctime );
    GetSystemTime( &systime );
  } while ( utctime != time( NULL ) );

  os_local_time.hi = (s64) utctime;
  os_local_time.lo = 0x004189374BC6A7EFLL;
  os_local_time.lo *= (u64) systime.wMilliseconds;
#else
  /* Query the OS for current time */
  if ( gettimeofday( &time_value, NULL ) != 0)
  {
    return COSM_FAIL;
  }

  /* Transform OS time to cosmtime format */
  /* 2^64 / 1000000 = 0x000010C6F7A0B5ED */

  os_local_time.hi = (u64) time_value.tv_sec;
  os_local_time.lo = 0x000010C6F7A0B5EDLL;
  os_local_time.lo = (u64) time_value.tv_usec * os_local_time.lo;

#endif
  /*
    gettimeofday gives the number of second since 1, January 1970
    In this count, there is no leap seconds, and all years divisible by 4
    are leap. So between 01/01/1970 and 01/01/2000 there is
    ( ( ( 366 * 7 ) + (365 * 23 ) ) * ( 24 * 60 * 60 ) ) =
    946.684.800 ( = 0x386D4380 ) seconds
  */
  _COSM_SET128( unix_epoc, 00000000386D4380, 0000000000000000 );

  *local_time = CosmS128Sub( os_local_time, unix_epoc );

  return COSM_PASS;
}

/* low level functions */

u8 Cosm_PriorityToCosm( int pri )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* 1-9 -> 1-255 */
  return (u8) ( ( ( pri - 1 ) * 31 ) + 1 );
#else /* OS */
  int ratio;

  /* Someone made UNIX's PRIO_MAX the low priority, Cosm has it straight */
  if ( ( pri > PRIO_MAX ) || ( pri < 0 ) )
  {
    /* out of our known range */
    return 0;
  }
  /* 20-0 -> 1-255 */
  ratio = ( PRIO_MAX * 1000 ) / ( COSM_PRI_MAX - COSM_PRI_MIN );
  return COSM_PRI_MAX - ( ( pri * 1000 ) / ratio );
#endif
}

int Cosm_PriorityToOS( u8 pri )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* 1-255 -> 1-9 */
  return (u8) ( ( pri / 31 ) + 1 );
#else
  int ratio;

  /* Someone made UNIX's PRIO_MAX the low priority, Cosm has it straight */
  /* 1-255 -> 20-0 */
  ratio = ( ( COSM_PRI_MAX - COSM_PRI_MIN ) * 1000 ) / PRIO_MAX;
  return PRIO_MAX - ( ( pri * 1000 ) / ratio );
#endif
}

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
void Cosm_SignalWaitThread( void * arg )
{
  HANDLE handles[2];
  u32 go;

  handles[0] = win32_sigint_handle;
  handles[1] = win32_sigterm_handle;

  for ( ; ; )
  {
    /* wait for the signals */
    go = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    switch ( go )
    {
      case WAIT_OBJECT_0:
        raise( SIGTERM );
        break;
      case ( WAIT_OBJECT_0 + 1 ):
        raise( SIGINT );
        break;
      case WAIT_TIMEOUT:
        /* odd, but OK */
        break;
      case WAIT_ABANDONED_0:
      case ( WAIT_ABANDONED_0 + 1 ):
        /* Documented as very very bad */
        CosmPrint( "Exit due to thread death in signal mutex\n" );
        /* fall through */
      case WAIT_FAILED:
        CosmProcessEnd( -1 );
        break;
    }
  }
}
#endif /* win32 */

/* test code */

#define COSM_THREAD_BUFFERSIZE  64
#define COSM_THREAD_MAXCOUNTER  255

typedef struct thread_data
{
  u64 tid_src;
  u64 tid_dest;
  u8 buffer[COSM_THREAD_BUFFERSIZE];
  u32 head;
  u32 tail;
  s32 exit_src;
  s32 exit_dest;
  cosm_MUTEX lock;
} thread_DATA;

void Cosm_ThreadTestSrc( void * arg )
{
  thread_DATA *data;
  u32 done;
  u32 count;

  data = (thread_DATA *) arg;

  data->tid_src = CosmThreadID();
  done = 0;
  count = 0;

  while ( done == 0 )
  {
    if ( CosmMutexLock( &data->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return;
    }
    if ( data->exit_dest != 0 )
    {
      /* noone to receive data */
      data->exit_src = -2;
      done = 1;
    }
    /* fill buffer */
    while ( ( ( ( data->tail + 1 ) % COSM_THREAD_BUFFERSIZE ) != data->head ) &&
      ( count < COSM_THREAD_MAXCOUNTER ) )
    {
      data->buffer[data->tail] = (u8) count++;
      data->tail = ( data->tail + 1 ) % COSM_THREAD_BUFFERSIZE;
      if ( count == COSM_THREAD_MAXCOUNTER )
      {
        data->exit_src = 1;
        done = 1;
      }
    }
    CosmMutexUnlock( &data->lock );
    CosmYield();
  }

  CosmThreadEnd();
}

void Cosm_ThreadTestDest( void * arg )
{
  thread_DATA *data;
  u32 done;
  u32 count;
  u8 value;

  data = (thread_DATA *) arg;

  data->tid_dest = CosmThreadID();
  done = 0;
  count = 0;

  while ( done == 0 )
  {
    if ( CosmMutexLock( &data->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return;
    }
    /* empty buffer */
    while ( ( data->head != data->tail )
      && ( count < COSM_THREAD_MAXCOUNTER ) )
    {
      value = data->buffer[data->head];
      if ( value != count++ )
      {
        data->exit_dest = -2;
        done = 1;
      }
      data->head = ( data->head + 1 ) % COSM_THREAD_BUFFERSIZE;
    }
    if ( count == COSM_THREAD_MAXCOUNTER )
    {
      data->exit_dest = 1;
      done = 1;
    }
    if ( data->exit_src != 0 )
    {
      /* noone to receive send more data? */
      if ( done != 1 )
      {
        /* if we're not already done that is bad */
        data->exit_src = -2;
        done = 1;
      }
    }
    CosmMutexUnlock( &data->lock );
    CosmYield();
  }

  CosmThreadEnd();
}

s32 Cosm_TestOSTask( void )
{
  thread_DATA data;
  u64 thread_id[2];
  u32 done;
  cosm_MUTEX mutex;
  cosm_SEMAPHORE semaphore, semaphore2;
  cosm_SEMAPHORE_NAME semaphore_name;
  u32 cpu_count;

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) \
  || ( OS_TYPE == OS_SOLARIS ) || ( OS_TYPE == LINUX ) )
  u32 cpu;
#endif

  /* Mutex tests */

  CosmMemSet( &mutex, sizeof( mutex ), 0 );
  if ( CosmMutexInit( &mutex ) != COSM_PASS )
  {
    return -1;
  }
  if ( CosmMutexLock( &mutex, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return -2;
  }
  CosmMutexUnlock( &mutex );
  if ( CosmMutexLock( &mutex, COSM_MUTEX_NOWAIT ) != COSM_PASS )
  {
    return -3;
  }
  CosmMutexUnlock( &mutex );
  CosmMutexFree( &mutex );

  /* Semaphore tests, 2 Downs only, then we should fail. */

  CosmMemSet( &semaphore, sizeof( semaphore ), 0 );
  if ( CosmSemaphoreInit( &semaphore, &semaphore_name, 2 ) != COSM_PASS )
  {
    return -4;
  }
  if ( CosmSemaphoreDown( &semaphore, COSM_SEMAPHORE_NOWAIT ) != COSM_PASS )
  {
    return -5;
  }
  if ( CosmSemaphoreDown( &semaphore, COSM_SEMAPHORE_WAIT ) != COSM_PASS )
  {
    return -6;
  }
  if ( CosmSemaphoreDown( &semaphore, COSM_SEMAPHORE_NOWAIT ) != COSM_FAIL )
  {
    return -7;
  }
  if ( CosmSemaphoreUp( &semaphore ) != COSM_PASS )
  {
    return -8;
  }
  if ( CosmSemaphoreDown( &semaphore, COSM_SEMAPHORE_NOWAIT ) != COSM_PASS )
  {
    return -9;
  }
  CosmMemSet( &semaphore2, sizeof( semaphore2 ), 0 );
  if ( COSM_PASS != CosmSemaphoreOpen( &semaphore2, &semaphore_name ) )
  {
    return -10;
  }
  CosmSemaphoreClose( &semaphore2 );
  CosmSemaphoreFree( &semaphore );

  /* Tests the Thread functions */
  thread_id[0] = 0x0000000000000000LL;
  thread_id[1] = 0x0000000000000000LL;
  CosmMemSet( &data, sizeof( thread_DATA ), 0 );

  /* tell things not to proceed yet */
  CosmMutexInit( &data.lock );
  if ( CosmMutexLock( &data.lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return -13;
  }

  /* Test for bad thread creation */
  if ( CosmThreadBegin( &thread_id[0], Cosm_ThreadTestSrc,
    (void *) &data, 4096 ) != COSM_PASS )
  {
    return -14;
  }

  /* Yield enough to let the TestSrc thread run */
  CosmYield();
  CosmSleep( 50 );
  CosmYield();

  /* TestSrc thread _shouldn't_ begin until we unlock the mutex */
  if ( data.tail != data.head )
  {
    return -15;
  }
  CosmMutexUnlock( &data.lock );

  /* Test for bad thread creation */
  if ( CosmThreadBegin( &thread_id[1], Cosm_ThreadTestDest,
    (void *) &data, 4096 ) != COSM_PASS )
  {
    return -16;
  }

  /* threads are running, wait for them to finish */
  done = 0;
  while ( done == 0 )
  {
    if ( CosmMutexLock( &data.lock, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return -17;
    }
    if ( ( data.exit_src != 0 ) && ( data.exit_dest != 0 ) )
    {
      done = 1;
    }
    CosmMutexUnlock( &data.lock );
    CosmYield();
  }

  if ( data.exit_src != 1 )
  {
    return -19;
  }

  if ( data.exit_dest != 1 )
  {
    return -20;
  }

  /* Test for CosmThreadID being correct */
  if ( data.tid_src != thread_id[0] ||
    data.tid_dest != thread_id[1] )
  {
    return -21;
  }

  /* test CPUCount in testlib */

  /* Test CPULock and Unlock */

  /* some things that should always fail */

  if ( ( CosmCPUCount( &cpu_count ) == COSM_FAIL ) && ( cpu_count != 1 ) )
  {
    return -22;
  }
  if ( CosmCPULock( (u64) 0, (u32) -1 ) != COSM_FAIL )
  {
    return -23;
  }
  if ( CosmCPULock( CosmProcessID(), (u32) -1 ) != COSM_FAIL )
  {
    return -24;
  }
  if ( CosmCPUUnlock(  0 ) != COSM_FAIL )
  {
    return -25;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) \
  || ( OS_TYPE == OS_SOLARIS ) || ( OS_TYPE == LINUX ) )
  /* we should be able to lock and unlock */
  for ( cpu = 0 ; cpu < cpu_count ; cpu++ )
  {
    if ( CosmCPULock( CosmProcessID(), cpu ) != COSM_PASS )
    {
      return -26;
    }
  }
  if ( CosmCPUUnlock( CosmProcessID() ) != COSM_PASS )
  {
    return -27;
  }
#else /* no CPULock/Unlock */
  if ( CosmCPULock( CosmProcessID(), 0 ) != COSM_FAIL )
  {
    return -28;
  }
  if ( CosmCPUUnlock( CosmProcessID() ) != COSM_FAIL )
  {
    return -29;
  }
#endif /* CPULock/Unlock */

#if ( ( OS_TYPE != OS_WIN32 ) && ( OS_TYPE == OS_WIN64 ) )
#if ( defined( COSM_PROCESSID_64 ) )
  if ( sizeof( pid_t ) != 8 )
#else
  if ( sizeof( pid_t ) != 4 )
#endif
  {
    return -30;
  }
#endif

  return COSM_PASS;
}
