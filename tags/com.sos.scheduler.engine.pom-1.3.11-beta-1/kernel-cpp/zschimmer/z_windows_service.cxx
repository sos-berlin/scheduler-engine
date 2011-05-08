// $Id$

/*
    ! pid jeder vmware-vmx.exe merken. Machine läuft nicht, wenn der Prozess beendet ist.
*/

#include "zschimmer.h"
#include "z_windows.h"
#include "z_windows_service.h"
#include "log.h"

#include <process.h>

using namespace std;

namespace zschimmer {
namespace windows {

//-----------------------------------------------------------------------------------Service_handle

struct Service_handle
{
                                Service_handle              ( SC_HANDLE h = NULL )          : _handle(h) {}
                               ~Service_handle              ()                              { close(); }

        void                    operator =                  ( SC_HANDLE h )                 { set_handle( h ); }
                                operator SC_HANDLE          () const                        { return _handle; }
        bool                    operator !                  () const                        { return _handle == 0; }

        void                    set_handle                  ( SC_HANDLE h )                 { close(); _handle = h; }
        SC_HANDLE               handle                      () const                        { return _handle; }
        void                    close                       ()                              { if(_handle) { CloseServiceHandle(_handle); _handle=NULL; } }

        SC_HANDLE              _handle;

  private:
                                Service_handle              ( const Service_handle& );      // Nicht implementiert
    void                        operator =                  ( const Service_handle& );      // Nicht implementiert
};


//-------------------------------------------------------------------------------------------static

static Service*                 static_service = NULL;

//----------------------------------------------------------------------------------------event_log

static void event_log( const string& service_name, const string& msg_par )
{
    string msg = "*** " + service_name + " *** " + msg_par;

    HANDLE h = RegisterEventSource( NULL, "Application" );
    const char* m = msg.c_str();
 
    Z_LOG( "ReportEvent()\n" );
    ReportEvent( h,                     // event log handle 
                 EVENTLOG_ERROR_TYPE,   // event type 
                 0,                     // category zero 
                 1,                     // event identifier ???
                 NULL,                  // no user security identifier 
                 1,                     // one substitution string 
                 (DWORD)msg.length(),   // no data 
                 &m,                    // pointer to string array 
                 (void*)msg.c_str() );  // pointer to data 

    DeregisterEventSource( h ); 

    //send_error_email( msg_par, argc, argv, "" );

    fprintf( stderr, "%s\n", msg.c_str() );
}

//----------------------------------------------------------------------------------install_service

void install_service( const string& service_name, const string& service_display, const string& service_description, const string& dependencies, const string& params ) 
{ 
    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin( "OpenSCManager" );


    char path[ 1024+1 ];
    int len = GetModuleFileName( NULL, (LPSTR)path, sizeof path );
    if( !len )  throw_mswin( "GetModuleFileName" );


    string command_line = quoted_windows_process_parameter( path );
    if( !params.empty() )  command_line += " " + params;

    Z_LOG( "CreateService(,\"" << service_name << "\", \"" << service_display << "\",,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,"
                        "SERVICE_ERROR_NORMAL,\"" << command_line << "\",,,,)\n" );
    SC_HANDLE service_handle = CreateService( 
                                    manager_handle,            // SCManager database 
                                    service_name.c_str(),      // name of service 
                                    service_display.c_str(),   // service name to display 
                                    SERVICE_ALL_ACCESS,        // desired access 
                                    SERVICE_WIN32_OWN_PROCESS, // service type 
                                    SERVICE_DEMAND_START,      // start type   oder SERVICE_AUTO_START
                                    SERVICE_ERROR_NORMAL,      // error control type 
                                    command_line.c_str(),      // service's binary 
                                    NULL,                      // no load ordering group 
                                    NULL,                      // no tag identifier 
                                    dependencies.data(),       // dependencies 
                                    NULL,                      // LocalSystem account 
                                    NULL );                    // no password 

    if( !service_handle )  throw_mswin( "CreateService" );


    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwMajorVersion >= 5 )     // Windows 2000?
    {
        SERVICE_DESCRIPTION d;
        d.lpDescription = (char*)service_description.c_str();
        Z_LOG( "ChangeServiceConfig2(,SERVICE_CONFIG_DESCRIPTION,\"" << service_description << "\")\n" );
        ChangeServiceConfig2( service_handle, SERVICE_CONFIG_DESCRIPTION, &d );
    }


    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//-----------------------------------------------------------------------------------remove_service

void remove_service( const string& service_name ) 
{ 
    BOOL ok;

    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin( "OpenSCManager" );

    Z_LOG( "DeleteService(\"" << service_name << "\")\n" );

    SC_HANDLE service_handle = OpenService( manager_handle, service_name.c_str(), DELETE );
    if( !service_handle )  throw_mswin( "OpenService" );

    ok = DeleteService( service_handle );
    if( !ok )  throw_mswin( "DeleteService" );

    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//------------------------------------------------------------------------------------start_service

void start_service( const string& name )
{
    Service_handle manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );    if( !manager_handle )  throw_mswin( "OpenSCManager" );
    Service_handle service_handle = OpenService( manager_handle, name.c_str(), SERVICE_START );                    if( !service_handle )  throw_mswin( "OpenService" );
    BOOL           ok             = StartService( service_handle, 0, NULL );                                       if( !ok             )  throw_mswin( "StartService" );
}

//-------------------------------------------------------------------------------service_state_name

string service_state_name( int state )
{
    switch( state )  
    {
        case SERVICE_STOPPED        : return "SERVICE_STOPPED";       
        case SERVICE_PAUSED         : return "SERVICE_PAUSED";        
        case SERVICE_START_PENDING  : return "SERVICE_START_PENDING"; 
        case SERVICE_STOP_PENDING   : return "SERVICE_STOP_PENDING";  
        case SERVICE_RUNNING        : return "SERVICE_RUNNING";       
        default                     : return as_string( (int)state );
    }
}

//----------------------------------------------------------------------Service::set_service_status

void Service::set_service_status( int state, int checkpoint_value, int wait_hint )
{
    SERVICE_STATUS  service_status;
    string          state_nam;

    if( !_service_status_handle )  return;

    service_status.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;        // | SERVICE_INTERACTIVE_PROCESS?

    service_status.dwCurrentState               = state; 
    service_status.dwControlsAccepted           = SERVICE_ACCEPT_STOP 
                                                | SERVICE_ACCEPT_PAUSE_CONTINUE 
                                                | SERVICE_ACCEPT_SHUTDOWN
                                                //| SERVICE_ACCEPT_PARAMCHANGE; 
                                                | SERVICE_ACCEPT_POWEREVENT;

    service_status.dwWin32ExitCode              = NO_ERROR;              
    service_status.dwServiceSpecificExitCode    = 0; 
    service_status.dwCheckPoint                 = checkpoint_value; 
    service_status.dwWaitHint                   = wait_hint;
                                                  //service_status.dwCurrentState == SERVICE_START_PENDING? (int)( _start_duration_hint*1000 ):
                                                  //service_status.dwCurrentState == SERVICE_STOP_PENDING ? (int)( _stop_duration_hint*1000 ) 
                                                  //                                                      : 0;

    // Nicht zu lange im Zustand xx_PENDING bleiben, weil die Knöpfe in der Diensteverwaltung gesperrt sind:
    /*
    if( service_status.dwCurrentState == SERVICE_START_PENDING )
    {
        if( _pending_timed_out )     // Uhr abgelaufen?
        {
            service_status.dwCurrentState = SERVICE_PAUSED;
        }
        else
        if( !_pending_watchdog_signal )
        {
            Thread_id thread_id;
            _pending_watchdog_signal = CreateEvent( NULL, FALSE, FALSE, "" );
            _beginthreadex( NULL, 0, pending_watchdog_thread, NULL, 0, &thread_id );   // Uhr aufziehen
        }
    }
    else
    {
        _pending_timed_out = false;
        if( _pending_watchdog_signal )  SetEvent( _pending_watchdog_signal );
    }
    */

    _state = service_status.dwCurrentState;

    Z_LOG( "SetServiceStatus " << service_state_name(service_status.dwCurrentState) << " waitHint=" << service_status.dwWaitHint << '\n' );
    BOOL ok = SetServiceStatus( _service_status_handle, &service_status );
    if( !ok )
    {
        //try { throw_mswin( "SetServiceStatus", state_name(service_status.dwCurrentState).c_str() ); }
        //catch( const exception& x ) { SHOW_MSG(x.what()); }       // Was tun?
    }

    if( service_status.dwCurrentState == SERVICE_STOPPED )
    {
        _service_status_handle = NULL;  // Sonst beim nächsten Mal: Fehler MSWIN-00000006  Das Handle ist ungültig. [SetServiceStatus] [SERVICE_STOPPED]
        //start_self_destruction();
    }
}

//----------------------------------------------------------------------string_from_handler_control

static string string_from_handler_control( DWORD c )
{
    switch( c )
    {
        case SERVICE_CONTROL_STOP:              return "SERVICE_CONTROL_STOP";
        case SERVICE_CONTROL_PAUSE:             return "SERVICE_CONTROL_PAUSE";
        case SERVICE_CONTROL_CONTINUE:          return "SERVICE_CONTROL_CONTINUE";
        case SERVICE_CONTROL_INTERROGATE:       return "SERVICE_CONTROL_INTERROGATE";
        case SERVICE_CONTROL_SHUTDOWN:          return "SERVICE_CONTROL_SHUTDOWN";
        case SERVICE_CONTROL_PARAMCHANGE:       return "SERVICE_CONTROL_PARAMCHANGE";
        case SERVICE_CONTROL_NETBINDADD:        return "SERVICE_CONTROL_NETBINDADD";
        case SERVICE_CONTROL_NETBINDREMOVE:     return "SERVICE_CONTROL_NETBINDREMOVE";
        case SERVICE_CONTROL_NETBINDENABLE:     return "SERVICE_CONTROL_NETBINDENABLE";
        case SERVICE_CONTROL_NETBINDDISABLE:    return "SERVICE_CONTROL_NETBINDDISABLE";
        default:                                return "SERVICE_CONTROL_" + as_string( c );
    }
}

//----------------------------------------------------------Service::self_destruction_thread_static

uint __stdcall Service::self_destruction_thread_static( void* context )
{
    ((Service*)context)->self_destruction_thread();
    return 0;
}

//-----------------------------------------------------------------Service::self_destruction_thread

uint Service::self_destruction_thread()
{
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    Z_LOG( "Selbstzerstörung in " << _self_destruction_timeout << " Sekunden (ohne weitere Ankündigung) ...\n" );

    sleep( _self_destruction_timeout );      // exit() sollte diesen Thread abbrechen. 25.11.2002

    
    // Jetzt ganz schnell, ohne Log, beenden!

    try
    {
        abort_immediately();
    }
    catch(...) {}

    TerminateProcess( GetCurrentProcess(), 1 );
    return 0;
}

//------------------------------------------------------------------Service::start_self_destruction

void Service::start_self_destruction()
{
    if( !_self_destruction_thread_id )  
    {
        _beginthreadex( NULL, 0, self_destruction_thread_static, NULL, 0, &_self_destruction_thread_id );

/* Erst ab Windows 2000:
        HANDLE h = OpenThread( THREAD_QUERY_INFORMATION, false, self_destruction_thread_id  );
        SetThreadPriority( h, THREAD_PRIORITY_HIGHEST );
        SwitchToThread();
        CloseThread( h );
*/
    }
}

//----------------------------------------------------------Service::static_service_control_handler

DWORD __stdcall Service::service_control_handler_static( DWORD dwControl, DWORD event_type, void* data, void* context )
{
    return ((Service*)context)->service_control_handler_super( dwControl, event_type, data );
}

//-----------------------------------------------------------Service::service_control_handler_super

DWORD Service::service_control_handler_super( DWORD dwControl, DWORD event_type, void* data )
{
    Z_LOGI( "\nservice_control_handler(" << string_from_handler_control(dwControl) << ")\n" );


    if( _self_destruction_timeout )
    {
        if( dwControl == SERVICE_CONTROL_STOP 
         || dwControl == SERVICE_CONTROL_SHUTDOWN )  start_self_destruction();
    }


    switch( dwControl )
    {
        case SERVICE_CONTROL_STOP:              // Requests the service to stop.  
        {
            _pending_timed_out = false;
            _service_stop = true;
            break;
        }

        case SERVICE_CONTROL_PAUSE:             // Requests the service to suspend.  
            _pending_timed_out = false;
            _service_stop = false;
            break;

        case SERVICE_CONTROL_CONTINUE:          // Requests the paused service to resume.  
            _pending_timed_out = false;
            _service_stop = false;
            break;

        case SERVICE_CONTROL_INTERROGATE:       // Requests the service to update immediately its current status information to the service control manager.  
            break;

        case SERVICE_CONTROL_SHUTDOWN:          // Requests the service to perform cleanup tasks, because the system is shutting down. 
            _pending_timed_out = false;
            _service_stop = true;
            break;

        case SERVICE_CONTROL_PARAMCHANGE:       // Windows 2000: Notifies the service that service-specific startup parameters have changed. The service should reread its startup parameters. 
            break;

        default:
            break;
    }

    return service_control_handler( dwControl, event_type, data );

    //set_service_status( 0 );
}

//-------------------------------------------------------------------Service::service_thread_static

uint __stdcall Service::service_thread_static( void* context )
{
    return ((Service*)context)->service_thread();
}

//--------------------------------------------------------------------------Service::service_thread

uint Service::service_thread()
{
    Z_LOGI( "service_thread\n" );

    int ret = 0;

    //set_service_status( SERVICE_START_PENDING );

    try
    {
        Z_LOG( "run()\n" );

        run( _argc, _argv );
    }
    catch( const exception& x )
    {
        if( _self_destruction_timeout )  start_self_destruction();

        set_service_status( SERVICE_PAUSED );     // Das schaltet die Diensteknöpfe frei
        event_log( _name, x.what() );
        ret = 99;
    }

    set_service_status( SERVICE_STOPPED );
    Z_LOG( "service_thread ok\n" );

    return ret;
}

//--------------------------------------------------------------------------------------ServiceMain

void __stdcall Service::ServiceMain( DWORD argc, char** argv )
{
    static_service->service_main( argc, argv );
}

//----------------------------------------------------------------------------Service::service_main

void Service::service_main( DWORD argc, char** argv )
{
    Z_LOGI( "ServiceMain(argc=" << argc << ")\n" );

    try
    {
        if( argc > 1 )
        {
            _argc = argc;   // Parameter des Dienstes? (die sind wohl nur zum Test)
            _argv = argv;                 
        }

        Z_LOG( "RegisterServiceCtrlHandler\n" );
        _service_status_handle = RegisterServiceCtrlHandlerEx( _name.c_str(), &Service::service_control_handler_static, this );
        if( !_service_status_handle )  throw_mswin( "RegisterServiceCtrlHandler" );
/*
        Thread_id thread_id;
        int       ret;

        Z_LOG( "CreateThread\n" );
        _thread_handle.set_handle( (HANDLE)_beginthreadex( NULL, 0, service_thread_static, this, 0, &thread_id ) );
        if( !_thread_handle )  throw_mswin( "CreateThread" );

        while(1)
        {
            Z_LOG( "MsgWaitForMultipleObjects()\n" );
            ret = MsgWaitForMultipleObjects( 1, &_thread_handle._handle, FALSE, INFINITE, QS_ALLINPUT ); 
        
            if( ret != WAIT_OBJECT_0 + 1 )  break;

            windows_message_step();
        }

        if( _abort_immediately )     // Gesetzt vom self_destruction_thread
        { 
            Z_LOG( "TerminateProcess()\n" ); 
            TerminateProcess( GetCurrentProcess(), 1 ); 
        }   

        TerminateThread( _thread_handle, 999 );   // Sollte nicht nötig sein. Nützt auch nicht, weil Destruktoren nicht gerufen werden und Komnunikations-Thread vielleicht noch läuft.
*/
        service_thread();

        Z_LOG( "ServiceMain ok\n" );
    }
    catch( const exception& x ) 
    { 
        event_log( _name, x.what() ); 
    }

    _service_main_terminated.async_signal( "terminated" );
}

//-----------------------------------------------------------------------------Service::run_service

int Service::run_service( const string& service_name, int argc, char** argv )
{
    Z_LOGI( "run_service()\n" );

    bool result = 0;

    static_service = this;

    try 
    {            
        _name = service_name;
        _argc = argc;
        _argv = argv;
        
        SERVICE_TABLE_ENTRY ste[2];
        memset( ste, 0, sizeof ste );
        ste[0].lpServiceName = const_cast<char*>( service_name.c_str() );
        ste[0].lpServiceProc = ServiceMain;

        Z_LOGI( "StartServiceCtrlDispatcher(" << ste[0].lpServiceName << ")\n" );

        BOOL ok = StartServiceCtrlDispatcher( ste );
        if( ok )
        {
            Z_LOG( "StartServiceCtrlDispatcher() OK\n" );
        }
        else
        {
            int error = GetLastError();
            if( error != 0x427  ||  !_service_emulation_allowed  ||  GetConsoleWindow() == NULL )  throw_mswin( error, "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.

            run( argc, argv );
        }

        WaitForSingleObject( (HANDLE)_service_main_terminated, INFINITE );
        //Z_MUTEX( _ServiceMain_mutex ) {}      // Warten, bis Thread ServiceMain sich beendet hat, erst dann diesen Mainthread beenden 
    }
    catch( const exception& x )
    {
        event_log( _name, x.what() );
    }

    static_service = NULL;

    Z_LOG( "service() fertig\n" );
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer
