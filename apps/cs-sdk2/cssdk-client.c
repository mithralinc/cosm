/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm CS-SDK2 - Client Server Software Development Kit

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  2009-2012 by Creator. All rights reserved. Further information about the
  Package can be found at the Creator's web site:
  http://www.mithral.com/
*/

/*
  Threads:

  Pinger (300)
    - Accept ping requests on 300
    - Pong immediately.

  Tracker
    - Generates Check-in/out.
    - Listen for reply with IP.

234512345: Checkin: Restart (127.0.0.1)
123512351: Pingpong. (127.0.0.1)
123512351: Pingpong. (127.0.0.1)
123512351: Pingpong. (127.0.0.1)
123512351: Pingpong. (127.0.0.1)
123412341: Checkin: moved? (127.0.0.2)
234512345: Checkin: Shutdown

Restart
Wait for pings.
  If no pings lately:
    Send a moved checkin
Shutdown
*/

#include "cssdk.h"

/* Are Windows clients going to run as a service? 0/1 */
#define SERVICE_MODE 0

void Notice( void );
void Usage( void );
void CatchINT( int arg );
void CatchTERM( int arg );

u32 __shutdown_flag = 0;

void Notice( void )
{
  cosmtime mytime;
  cosm_TIME_UNITS myunits;

  CosmSystemClock( &mytime );
  CosmTimeUnitsGregorian( &myunits, mytime );

  CosmPrint(
    "Cosm CS-SDK2 Client version %u.\n"
    "Copyright Mithral Communications & Design, Inc. 2009-%j.\n"
    "Visit http://www.mithral.com/ for more information.\n\n"
    "EXPIRES ----- %i-%02i-%02i ----- EXPIRES (Midnight yyyy-mm-dd UTC)\n\n",
    (u32) CSSDK_VER_MAJOR, myunits.year,
    EXPIRE_YEAR, EXPIRE_MONTH, EXPIRE_DAY );
}

void Usage( void )
{
#if ( ( SERVICE_MODE == 1 ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) ) )
  CosmPrint(
    "Windows install:\n"
    "  cssdk-client.exe install machine_id -d work_directory\n"
    "To start the service (automatic after first time)\n"
    "  net start cssdk-client\n"
    "To stop the service\n"
    "  net stop cssdk-client\n"
    "Windows service uninstall: (does NOT stop it, so stop it first)\n"
    "  cssdk-client.exe uninstall\n"
    "\n"
    );
#else
  CosmPrint(
    "UNIX Usage:\n"
    "  sudo ./cssdk-client.exe machine_id -d \"work_directory\"\n"
    "Your machine_id is given at the URL above. Don't foget the quotes.\n"
    "Example:\n"
    "  sudo ./cssdk-client.exe XXXXXXXX -d \"/Users/bob/cssdk/\"\n"
    "\n"
    );
#endif
}

void CatchINT( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_INT, CatchINT );
}

void CatchTERM( int arg )
{
  __shutdown_flag = 1;
  CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM );
}

int CSSDKMain( int argc, char * argv[] )
{
  cosmtime time_now, time_expire;
  cosm_TIME_UNITS expire_units;
  u32 id;
  cosm_LOG log;
  cosm_NET_ADDR listen;
  cosm_NET net;
  u32 current_minor = 1, updated_minor;

  cosm_DYNAMIC_LIB dylib;
  s32 (*engine)( u32, cosmtime, u32 *, cosm_LOG *, cosm_NET * );
  cssdk_engine_result result;

  Notice();

  /*  cssdk-client.exe hostname -d directory */
  if ( ( argc != 4 )
    || ( CosmU32Str( &id, NULL, argv[1], 16 ) != COSM_PASS )
    || ( ( CosmStrCmp( argv[2], "-d", 3 ) != 0 )
      && ( CosmStrCmp( argv[2], "-D", 3 ) != 0 ) ) )
  {
    Usage();
    return -1;
  }

  if ( ( CosmSignalRegister( COSM_SIGNAL_INT, CatchINT ) != COSM_PASS ) ||
    ( CosmSignalRegister( COSM_SIGNAL_TERM, CatchTERM ) != COSM_PASS ) )
  {
    return -2;
  }

  /* change directory */
  CosmDirSet( argv[3] );

  /* open log */
  CosmMemSet( &log, sizeof( log), 0 );
  if ( CosmLogOpen( &log, "cssdk-client.log", 3, COSM_LOG_MODE_NUMBER )
    != COSM_PASS )
  {
    CosmPrint( "Unable to open log.\n" );
    return -3;
  }

  /* expire date */
  CosmMemSet( &expire_units, sizeof( expire_units ), 0 );
  expire_units.year = EXPIRE_YEAR;
  expire_units.month = EXPIRE_MONTH - 1;
  expire_units.day = EXPIRE_DAY - 1;
  CosmTimeDigestGregorian( &time_expire, &expire_units );
  CosmSystemClock( &time_now );
  if ( CosmS128Gt( time_now, time_expire ) )
  {
    CosmLog( &log, 0, COSM_LOG_ECHO,
      "%.32sThis program expired on %04j-%02u-%02u "
      "Visit %.128s for updates or information.\n\n ",
      Now(), expire_units.year, expire_units.month + 1,
      expire_units.day + 1, CSSDK_STATS );
    CosmLogClose( &log );
    return -4;
  }

  CosmLog( &log, 0, COSM_LOG_ECHO,
    "%.32sSoftware expires in %j days, %j:%02j:%02j\n", Now(),
    ( time_expire.hi - time_now.hi ) / 86400,
    ( time_expire.hi - time_now.hi ) % 86400 / 3600,
    ( time_expire.hi - time_now.hi ) % 3600 / 60,
    ( time_expire.hi - time_now.hi ) % 60 );

  /* listening address 0.0.0.0 */
  listen.type = COSM_NET_IPV4;
  listen.ip.v4 = 0;
  listen.port = CSSDK_PORT_CLIENT;

  CosmMemSet( &net, sizeof( net ), 0 );
  if ( CosmNetListen( &net, &listen, COSM_NET_MODE_UDP, 8 ) != COSM_PASS )
  {
    CosmLog( &log, 0, COSM_LOG_ECHO,
      "%.32sUnable to open listening socket\n\n", Now() );
    CosmLogClose( &log );
    return -5;
  }

/* If you want online updates of the engine... */
#if 0
  /* Load up the engine in the current directory */
  if ( UpdateFile( "./", "cssdk-engine.dylib", &updated_minor,
    CSSDK_VER_MAJOR, current_minor, &log ) != COSM_PASS )
  {
    CosmLog( &log, 0, COSM_LOG_ECHO,
      "%.32sUnable to download and verify cssdk-engine.dylib\n\n", Now() );
    CosmLogClose( &log );
    return -6;
  }
  else if ( updated_minor != current_minor )
  {
    CosmLog( &log, 0, COSM_LOG_ECHO,
      "%.32sUpdated cssdk-engine.dylib to version %u.%u\n", Now(),
      CSSDK_VER_MAJOR, updated_minor );
    current_minor = updated_minor;
  }
#endif

  /* call the engine */
  CosmMemSet( &dylib, sizeof( cosm_DYNAMIC_LIB ), 0 );
  if ( COSM_PASS != CosmDynamicLibLoad( &dylib, "./cssdk-engine.dylib" ) )
  {
    CosmLog( &log, 0, COSM_LOG_ECHO,
      "%.32sFailed to load dynamic library.\n\n", Now() );
    CosmLogClose( &log );
    return -7;
  }
  else
  {
    if ( ( engine = CosmDynamicLibGet( &dylib, "CSSDKEngine" ) ) == NULL )
    {
      CosmPrint( "Failed to load CSSDKEngine function.\n" );
      CosmLogClose( &log );
      return -7;
    }
    
    /* Call the engine, and release it */
    result = (*engine)( id, time_expire, &__shutdown_flag, &log, &net );
    CosmDynamicLibFree( &dylib );

    /* !!! deal with the result */
    switch ( result )
    {
      case CSSDK_ENGINE_SHUTDOWN: /* normal */
        break;
      case CSSDK_ENGINE_EXPIRED_CLIENT: /* needs to be reinstalled */
        break;
      case CSSDK_ENGINE_EXPIRED_ENGINE: /* get a new one */
        break;
      case CSSDK_ENGINE_NETWORK:
        break;
    }
    
  }

  return COSM_PASS;
}

/* Windows Service Code =================================================== */
#if ( ( SERVICE_MODE == 1 ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) ) )

#define SERVICE_NAME        "rename-me-cs-sdk-client"
#define SERVICE_DISPLAY     "Rename Me Cosm CS-SDK Client"
#define SERVICE_DESC        "Fix this - Monitors host availability."

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
int __win_argc = 0;
char ** __win_argv = NULL;

void ServiceInstall( int argc, char * argv[] )
{
  SC_HANDLE hManager;
  SC_HANDLE hService;
  SERVICE_DESCRIPTION sd;
  ascii path[MAX_PATH], command[MAX_PATH];
  int i, error;

  if ( !GetModuleFileName( NULL, path, MAX_PATH ) )
  {
    CosmPrint( "Cannot install service (%i)\n", GetLastError() );
    return;
  }
  
  error = COSM_PASS;
  error += CosmStrCopy( command, "\"", 2 );
  error += CosmStrAppend( command, path, MAX_PATH );
  error += CosmStrAppend( command, "\"", MAX_PATH );
  for ( i = 2 ; i < argc ; i++ )
  {
    error += CosmStrAppend( command, " \"", MAX_PATH );
    error += CosmStrAppend( command, argv[i], MAX_PATH );
    error += CosmStrAppend( command, "\"", MAX_PATH );
  }
  if ( error != COSM_PASS )
  {
    CosmPrint( "Paths were too long for Windows.\n" );
    return;    
  }

  if ( ( hManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) )
    == NULL )
  {
    CosmPrint( "OpenSCManager failed (%i).\n", GetLastError() );
    return;
  }

  /* Create the service */

  if ( ( hService = CreateService( hManager, /* SCM database */
    SERVICE_NAME,               /* name of service */
    SERVICE_DISPLAY,            /* service name to display */
    SERVICE_ALL_ACCESS,         /* desired access */
    SERVICE_WIN32_OWN_PROCESS,  /* | SERVICE_INTERACTIVE_PROCESS, */
    SERVICE_AUTO_START,         /* start type */
    SERVICE_ERROR_NORMAL,       /* error control type */
    command,                    /* path to service's binary */
    NULL, NULL,
    "Tcpip\0",                  /* Dependencies */
    NULL, NULL ) ) == NULL )
  {
    CosmPrint( "CreateService failed (%i).\n", GetLastError() );
    CloseServiceHandle( hManager );
    return;
  }
  else
  {
    CosmPrint( "Service installed.\n" );
  }

  /* Change the service description */
  sd.lpDescription = SERVICE_DESC;

  if( ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, &sd ) == 0 )
  {
    CosmPrint( "Unable to update service description.\n" );
  }

  CloseServiceHandle( hService );
  CloseServiceHandle( hManager );
}

void ServiceUninstall( int argc, char * argv[] )
{
  SC_HANDLE hManager;
  SC_HANDLE hService;

  if ( ( hManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) )
    == NULL )
  {
    CosmPrint( "OpenSCManager failed (%i).\n", GetLastError() );
    return;
  }

  if ( ( hService = OpenService( hManager, SERVICE_NAME, DELETE ) )
    == NULL )
  {
    CosmPrint( "OpenService failed (%i).\n", GetLastError() );
    CloseServiceHandle( hManager );
    return;
  }
  
  if ( ( DeleteService( hService ) == 0 )
    && ( GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE ) )
  {
    CosmPrint( "Unable to delete service.\n" );
    CloseServiceHandle( hManager );
    return;
  }
  else
  {
    CosmPrint( "Service uninstalled.\n" );
  }
}

void ServiceHandler( DWORD dwControl, DWORD dwEventType,
  LPVOID lpEventData, LPVOID lpContext )
{
  switch ( dwControl )
  {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
      ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
      __shutdown_flag = 1;
      break;    
  }

  ServiceStatus.dwCheckPoint++;
  SetServiceStatus( hStatus, &ServiceStatus );
}

void ServiceMain( int argc, char * argv[] )
{
  ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  ServiceStatus.dwControlsAccepted =
    SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
  ServiceStatus.dwWin32ExitCode = 0;
  ServiceStatus.dwServiceSpecificExitCode = 0;
  ServiceStatus.dwCheckPoint = 0;
  ServiceStatus.dwWaitHint = 5000;

  if ( ( hStatus = RegisterServiceCtrlHandlerEx( SERVICE_NAME,
    (LPHANDLER_FUNCTION_EX) ServiceHandler, NULL ) ) == NULL )
  {
    return;
  }
  
  /* we're running */
  ServiceStatus.dwCurrentState = SERVICE_RUNNING;
  SetServiceStatus( hStatus, &ServiceStatus );

  /* go run the program */
  ServiceStatus.dwWin32ExitCode = CSSDKMain( __win_argc, __win_argv );

  /* we're done */
  ServiceStatus.dwCurrentState = SERVICE_STOPPED;
  SetServiceStatus( hStatus, &ServiceStatus );
}

#endif
/* End Windows Service Code =============================================== */

int main( int argc, char * argv[] )
{
#if ( ( SERVICE_MODE == 1 ) \
  && ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) ) )
  /* service setup */
  SERVICE_TABLE_ENTRY DispatchTable[] =
  {
    { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
  };

  Notice();
  
  if ( argc < 2 )
  {
    Usage();
    return -1;
  }

  /* Windows Services stomp on argc/argv */
  __win_argc = argc;
  __win_argv = argv;
  /* install/uninstall */
  if ( CosmStrCmp( "install", argv[1], 10 ) == 0 )
  {
    ServiceInstall( argc, argv );
    return 0;
  }
  if ( CosmStrCmp( "uninstall", argv[1], 10 ) == 0 )
  {
    ServiceUninstall( argc, argv );
    return 0;
  }

  /* we need to step up the priority */
  SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

  /* when this returns the service has stopped */
  StartServiceCtrlDispatcher( DispatchTable );
#else
  return CSSDKMain( argc, argv );
#endif
}
