// $Id: spooler_service.cxx,v 1.4 2001/01/16 16:40:36 jz Exp $
/*
    Hier sind implementiert

    install_service()
    remove_service()
    service_is_started()
    spooler_service()
*/


#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/log.h"
#include "../kram/sosopt.h"

#ifdef SYSTEM_WIN 

#include <windows.h>

namespace sos {

string module_filename();

namespace spooler {

//--------------------------------------------------------------------------------------------const

static const char               service_name[]          = "DocumentFactory_Spooler";    // Windows 2000 Service
static const char               service_display_name[]  = "DocumentFactory Spooler";    // Gezeigter Name
static const char               service_description[]   = "Für die Hintergrund-Jobs der DocumentFactory";

//-------------------------------------------------------------------------------------------static

static Spooler*                 spooler_ptr;
SERVICE_STATUS_HANDLE           service_status_handle;
Handle                          thread_handle;

//-----------------------------------------------------------------------------Service_thread_param

struct Service_thread_param
{
    int                        _argc;
    char**                     _argv;
};

//----------------------------------------------------------------------------------------event_log

static void event_log( const string& msg )
{
    HANDLE h = RegisterEventSource( NULL, "Application" );
    const char* m = msg.c_str();
 
    ReportEvent( h,                     // event log handle 
                 EVENTLOG_ERROR_TYPE,   // event type 
                 0,                     // category zero 
                 1,                     // event identifier ???
                 NULL,                  // no user security identifier 
                 1,                     // one substitution string 
                 msg.length(),          // no data 
                 &m,                    // pointer to string array 
                 (void*)msg.c_str() );  // pointer to data 

    DeregisterEventSource( h ); 

    cerr << msg << '\n';
}

//----------------------------------------------------------------------------------install_service

void install_service() 
{ 
    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );


    string program_path = module_filename();

    SC_HANDLE service_handle = CreateService( 
                                    manager_handle,            // SCManager database 
                                    service_name,              // name of service 
                                    service_display_name,      // service name to display 
                                    SERVICE_ALL_ACCESS,        // desired access 
                                    SERVICE_WIN32_OWN_PROCESS, // service type 
                                    SERVICE_DEMAND_START,      // start type   oder SERVICE_AUTO_START
                                    SERVICE_ERROR_NORMAL,      // error control type 
                                    program_path.c_str(),      // service's binary 
                                    NULL,                      // no load ordering group 
                                    NULL,                      // no tag identifier 
                                    NULL,                      // no dependencies 
                                    NULL,                      // LocalSystem account 
                                    NULL);                     // no password 

    if( !service_handle )  throw_mswin_error( "CreateService" );


    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwMajorVersion >= 5 )     // Windows 2000?
    {
        SERVICE_DESCRIPTION d;
        d.lpDescription = (char*)service_description;
        ChangeServiceConfig2( service_handle, SERVICE_CONFIG_DESCRIPTION, &d );
    }


    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//-----------------------------------------------------------------------------------remove_service

void remove_service() 
{ 
    BOOL ok;

    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );

    SC_HANDLE service_handle = OpenService( manager_handle, service_name, DELETE );
    if( !service_handle )  throw_mswin_error( "OpenService" );

    ok = DeleteService( service_handle );
    if( !ok )  throw_mswin_error( "DeleteService" );

    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//-------------------------------------------------------------------------------service_is_started

bool service_is_started()
{
    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwPlatformId != VER_PLATFORM_WIN32_NT )  return false;


    bool      is_started     = false;
    SC_HANDLE manager_handle = 0;
    SC_HANDLE service_handle = 0;

    try 
    {
        BOOL ok;
        SERVICE_STATUS status;  memset( &status, 0, sizeof status );

        manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
        if( !manager_handle ) {
            if( GetLastError() == ERROR_ACCESS_DENIED          )  goto ENDE;
            throw_mswin_error( "OpenSCManager" );
        }

        service_handle = OpenService(manager_handle, service_name, SERVICE_QUERY_STATUS );
        if( !service_handle ) {
            if( GetLastError() == ERROR_ACCESS_DENIED          )  goto ENDE;
            if( GetLastError() == ERROR_INVALID_NAME           )  goto ENDE;
            if( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST )  goto ENDE;
            throw_mswin_error( "OpenService" );
        }

        ok = QueryServiceStatus( service_handle, &status );
        if( !ok ) {
            if( GetLastError() == ERROR_ACCESS_DENIED          )  goto ENDE;
            throw_mswin_error( "OpenService" );
        }

        if( status.dwCurrentState == SERVICE_START_PENDING )  is_started = true;

      ENDE:
        CloseServiceHandle( service_handle ),  service_handle = 0; 
        CloseServiceHandle( manager_handle ),  manager_handle = 0;
    }
    catch( const Xc& )
    {
        CloseServiceHandle( service_handle ); 
        CloseServiceHandle( manager_handle );
    }

    return is_started;
}

//----------------------------------------------------------------------------------service_handler

static void set_service_status( int spooler_error = 0 )
{
    if( !service_status_handle )  return;
    if( !spooler_ptr && spooler_error == 0 )  spooler_error = 1;

    SERVICE_STATUS service_status;

    service_status.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;

    service_status.dwCurrentState               = !spooler_ptr? SERVICE_STOPPED
                                                : spooler_ptr->_state == Spooler::s_stopped ? SERVICE_STOPPED
                                                : spooler_ptr->_state == Spooler::s_starting? SERVICE_START_PENDING
                                                : spooler_ptr->_state == Spooler::s_stopping? SERVICE_STOP_PENDING
                                                : spooler_ptr->_state == Spooler::s_running ? SERVICE_RUNNING
                                                : spooler_ptr->_state == Spooler::s_paused  ? SERVICE_PAUSED
                                                                                            : SERVICE_START_PENDING; 

    service_status.dwControlsAccepted           = SERVICE_ACCEPT_STOP 
                                                | SERVICE_ACCEPT_PAUSE_CONTINUE 
                                                | SERVICE_ACCEPT_SHUTDOWN 
                                                | SERVICE_ACCEPT_PARAMCHANGE; 
    
    service_status.dwWin32ExitCode              = spooler_error? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR;              
    service_status.dwServiceSpecificExitCode    = spooler_error; 
    service_status.dwCheckPoint                 = 0; 
    service_status.dwWaitHint                   = service_status.dwCurrentState == SERVICE_START_PENDING? 10*1000 : 0;  // 10s erlauben für den Start (wenn es ein Startskript mit DB-Verbindung gibt)

    LOG( "SetServiceStatus\n" );
    SetServiceStatus( service_status_handle, &service_status );
}

//------------------------------------------------------------------------------------------Handler

static void __stdcall Handler( DWORD dwControl )
{
    LOGI( "Service Handler(" << dwControl << ")\n" )

    if( spooler_ptr )
    {
        switch( dwControl )
        {
            case SERVICE_CONTROL_STOP:              // Requests the service to stop.  
                spooler_ptr->cmd_terminate();
                break;

            case SERVICE_CONTROL_PAUSE:             // Requests the service to pause.  
                spooler_ptr->cmd_pause();
                break;

            case SERVICE_CONTROL_CONTINUE:          // Requests the paused service to resume.  
                spooler_ptr->cmd_continue();
                break;

            case SERVICE_CONTROL_INTERROGATE:       // Requests the service to update immediately its current status information to the service control manager.  
                break;

            case SERVICE_CONTROL_SHUTDOWN:          // Requests the service to perform cleanup tasks, because the system is shutting down. 
                spooler_ptr->cmd_stop();
                break;

            case SERVICE_CONTROL_PARAMCHANGE:       // Windows 2000: Notifies the service that service-specific startup parameters have changed. The service should reread its startup parameters. 
                spooler_ptr->cmd_reload();
                break;
/*
            case SERVICE_CONTROL_NETBINDADD:        // Windows 2000: Notifies a network service that there is a new component for binding. The service should bind to the new component.  
            case SERVICE_CONTROL_NETBINDREMOVE:     // Windows 2000: Notifies a network service that a component for binding has been removed. The service should reread its binding information and unbind from the removed component.  
            case SERVICE_CONTROL_NETBINDENABLE:     // Windows 2000: Notifies a network service that a disabled binding has been enabled. The service should reread its binding information and add the new binding.  
            case SERVICE_CONTROL_NETBINDDISABLE:    // Windows 2000: Notifies a network service that one of its bindings has been disabled. The service should reread its binding information and remove the binding. 
*/
            default:
                break;
        }
    }


    set_service_status();
}

//-------------------------------------------------------------------------------------------------

static void spooler_state_changed( Spooler*, void* )
{
    set_service_status();
}

//-----------------------------------------------------------------------------------service_thread

static ulong __stdcall service_thread( void* param )
{
    LOG( "service_thread\n" );

    Ole_initialize ole;

    Service_thread_param* p   = (Service_thread_param*)param;
    int                   ret = 0;

    {
        Spooler spooler;
        spooler_ptr = &spooler;

        spooler._is_service = true;
        spooler.set_state_changed_handler( spooler_state_changed );
        set_service_status();

        try
        {
            LOG( "Spooler launch\n" );
            spooler_ptr->launch( p->_argc, p->_argv );
        }
        catch( const Xc& x )
        {
            event_log( x.what() );
            set_service_status(2);
            spooler._log.error( x.what() );
            spooler_ptr = NULL;
            ret = 1;
          //throw;
        }

        spooler_ptr = NULL;
    }

    set_service_status();

    return ret;
}

//--------------------------------------------------------------------------------------ServiceMain

static void __stdcall ServiceMain( DWORD argc, char** argv )
{
    LOGI( "ServiceMain(argc=" << argc << ")\n" );
    
    try
    {
        DWORD                   thread_id;
        Service_thread_param    param;
        int                     ret;

        for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
        {
            if( opt.with_value( "log"              ) )  log_start( opt.value() );
        }

        param._argc = argc;
        param._argv = argv;

        LOG( "RegisterServiceCtrlHandler\n" );
        service_status_handle = RegisterServiceCtrlHandler( service_name, &Handler );
        if( !service_status_handle )  throw_mswin_error( "RegisterServiceCtrlHandler" );

        LOG( "CreateThread\n" );
        thread_handle = CreateThread( NULL, 0, service_thread, &param, 0, &thread_id );
        if( !thread_handle )  throw_mswin_error( "CreateThread" );

        do ret = WaitForSingleObject( thread_handle, INT_MAX );  while( ret != WAIT_TIMEOUT );

        TerminateThread( thread_handle, 99 );   // Sollte nicht nötig sein. Nützt auch nicht, weil Destruktoren nicht gerufen werden und Komnunikations-Thread vielleicht noch läuft.
    }
    catch( const Xc& x        ) { event_log( x.what() ); }
    catch( const exception& x ) { event_log( x.what() ); }
}

//----------------------------------------------------------------------------------spooler_service

int spooler_service( int argc, char** argv )
{
    LOGI( "spooler_service(argc=" << argc << ")\n" );

    try 
    {
        SERVICE_TABLE_ENTRY ste[2];

        memset( ste, 0, sizeof ste );

        ste[0].lpServiceName = (char*)service_name;
        ste[0].lpServiceProc = ServiceMain;

        LOGI( "StartServiceCtrlDispatcher()\n" );

        BOOL ok = StartServiceCtrlDispatcher( ste );
        if( !ok )  throw_mswin_error( "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.
    }
    catch( const Xc& x )
    {
        event_log( x.what() );
        return 1;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif //SYSTEM_WIN