// $Id$
/*
    Hier sind implementiert

    install_service()
    remove_service()
    service_is_started()
    spooler_service()
*/


#include "spooler.h"
#ifdef Z_WINDOWS

#include "../kram/log.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../kram/msec.h"

#include <windows.h>
#include <process.h>

namespace sos {
namespace spooler {

using namespace zschimmer::windows;

//--------------------------------------------------------------------------------------------const

static const char               std_service_name[]          = "sos_scheduler";    // Windows 2000 Service
static const char               std_service_display_name[]  = "SOS Scheduler";    // Gezeigter Name
//static const char             service_description[]       = "Hintergrund-Jobs der Document Factory";
static const int                terminate_timeout           = 30;
static const int                stop_timeout                = terminate_timeout + 5;        // Zeit lassen, um Datenbank zu schließen, dann abort_now() per Thread
static const int                pending_timeout             = 60;

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

static bool                     terminate_immediately       = false;
static bool                     service_stop                = false;                        // STOP-Kommando von der Dienstesteuerung (und nicht von TCP-Schnittstelle)
static Thread_id                self_destruction_thread_id;

SERVICE_STATUS_HANDLE           service_status_handle;
Handle                          thread_handle;
string                          spooler_service_name;
int                             process_argc;
char**                          process_argv;

// Zustände SERVICE_START_PENDING und SERVICE_STOP_PENDING nicht länger als pending_timeout Sekunden:
Thread_semaphore                set_service_lock            ( "set_service" );
Thread_semaphore                ServiceMain_lock            ( "ServiceMain" );
Handle                          pending_watchdog_signal;
bool                            pending_timed_out;
int                             current_state;

//-------------------------------------------------------------------------------------------------

static void                     start_self_destruction      ();
static uint __stdcall           pending_watchdog_thread     ( void* );

//-----------------------------------------------------------------------------Service_thread_param

struct Service_thread_param
{
    int                        _argc;
    char**                     _argv;
};

//---------------------------------------------------------------------------------make_service_name

string make_service_name( const string& id )
{
    if( id.empty() )  return std_service_name;
                else  return std_service_name + ( "_" + id );
}

//------------------------------------------------------------------------------make_service_display

string make_service_display( const string& id )
{
    if( id.empty() )  return std_service_display_name;
                else  return string(std_service_display_name) + " -id=" + id;
}

//----------------------------------------------------------------------------------------event_log

static void event_log( const exception& x, int argc, char** argv, Spooler* spooler = NULL )
{
    string what = x.what();
    Z_LOG( "event_log() ERROR  " << what << "\n" );

    string msg = "*** SOS SCHEDULER *** " + what;

    HANDLE h = RegisterEventSource( NULL, "Application" );
    const char* m = msg.c_str();
 
    LOG( "ReportEvent()\n" );
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

    send_error_email( x, argc, argv, "", spooler );

    fprintf( stderr, "%s\n", msg.c_str() );
}

//----------------------------------------------------------------------------------install_service

void install_service( const string& service_name, const string& service_display, const string& service_description, const string& dependencies, const string& params ) 
{ 
    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );


    string command_line = quoted_command_parameter( program_filename() );
    if( !params.empty() )  command_line += " " + params;

    LOG( "CreateService(,\"" << service_name << "\", \"" << service_display << "\",,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,"
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

    if( !service_handle )  throw_mswin_error( "CreateService" );


    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwMajorVersion >= 5 )     // Windows 2000?
    {
        SERVICE_DESCRIPTION d;
        d.lpDescription = (char*)service_description.c_str();
        LOG( "ChangeServiceConfig2(,SERVICE_CONFIG_DESCRIPTION,\"" << service_description << "\")\n" );
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
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );

    LOG( "DeleteService(\"" << service_name << "\")\n" );

    SC_HANDLE service_handle = OpenService( manager_handle, service_name.c_str(), DELETE );
    if( !service_handle )  throw_mswin_error( "OpenService" );

    ok = DeleteService( service_handle );
    if( !ok )  throw_mswin_error( "DeleteService" );

    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//------------------------------------------------------------------------------------service_state

DWORD service_state( const string& service_name )
{
    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwPlatformId != VER_PLATFORM_WIN32_NT )  return false;


    DWORD     result         = 0;
    SC_HANDLE manager_handle = 0;
    SC_HANDLE service_handle = 0;

    try 
    {
        BOOL           ok;
        SERVICE_STATUS status;  memset( &status, 0, sizeof status );

        manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
        if( !manager_handle ) {
            if( GetLastError() == ERROR_ACCESS_DENIED          )  goto ENDE;
            throw_mswin_error( "OpenSCManager" );
        }

        service_handle = OpenService(manager_handle, service_name.c_str(), SERVICE_QUERY_STATUS );
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

        result = status.dwCurrentState;

      ENDE:
        CloseServiceHandle( service_handle ),  service_handle = 0; 
        CloseServiceHandle( manager_handle ),  manager_handle = 0;
    }
    catch( const exception& )
    {
        CloseServiceHandle( service_handle ); 
        CloseServiceHandle( manager_handle );
    }

    return result;
}

//------------------------------------------------------------------------------------service_start

void service_start( const string& service_name )
{
    BOOL            ok;
    Service_handle  manager_handle;
    Service_handle  service_handle;

    manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );
    service_handle = OpenService( manager_handle, service_name.c_str(), SERVICE_START );            if( !service_handle )  throw_mswin_error( "OpenService" );

    Z_LOG( "ServiceStart(\"" << service_name << "\")\n" );
    ok = StartService( service_handle, 0, NULL );                                                   if( !ok )  throw_mswin_error( "StartService" );
}

//---------------------------------------------------------------------------------------state_name

static string state_name( int state )
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

//----------------------------------------------------------------------------------service_handler

static void set_service_status( int spooler_error, int state = 0 )
{
    SERVICE_STATUS  service_status;
    string          state_nam;

    if( !service_status_handle )  return;

    THREAD_LOCK( set_service_lock )
    {
        DWORD stop_pending = service_stop? SERVICE_STOP_PENDING    // Nur, wenn Dienstesteuerung den Spooler beendet 
                                         : SERVICE_RUNNING;        // Wenn Spooler über TCP beendet wird, soll der Diensteknopf "beenden" frei bleiben. Deshalb paused.

        service_status.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;

        service_status.dwCurrentState               = state                                              ? state
                                                    : !spooler_ptr                                       ? SERVICE_STOPPED 
                                                    : spooler_ptr->state() == Spooler::s_stopped         ? SERVICE_STOPPED       //SetServiceStatus() ruft exit()!
                                                    : spooler_ptr->state() == Spooler::s_starting        ? SERVICE_START_PENDING
                                                    : spooler_ptr->state() == Spooler::s_stopping        ? stop_pending
                                                    : spooler_ptr->state() == Spooler::s_stopping_let_run? stop_pending
                                                    : spooler_ptr->state() == Spooler::s_running         ? SERVICE_RUNNING
                                                    : spooler_ptr->state() == Spooler::s_paused          ? SERVICE_PAUSED
                                                                                                         : SERVICE_START_PENDING; 
        service_status.dwControlsAccepted           = SERVICE_ACCEPT_STOP 
                                                    | SERVICE_ACCEPT_PAUSE_CONTINUE 
                                                    | SERVICE_ACCEPT_SHUTDOWN;
                                 // Nicht für NT 4: | SERVICE_ACCEPT_PARAMCHANGE; 
    
        service_status.dwWin32ExitCode              = spooler_error? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR;              
        service_status.dwServiceSpecificExitCode    = spooler_error; 
        service_status.dwCheckPoint                 = 0; 
        service_status.dwWaitHint                   = service_status.dwCurrentState == SERVICE_START_PENDING? 10*1000 :         // 10s erlauben für den Start (wenn es ein Startskript mit DB-Verbindung gibt)
                                                      service_status.dwCurrentState == SERVICE_STOP_PENDING ? stop_timeout*1000 
                                                                                                            : 0;

        // Nicht zu lange im Zustand xx_PENDING bleiben, weil die Knöpfe in der Diensteverwaltung gesperrt sind:
        if( service_status.dwCurrentState == SERVICE_START_PENDING )
        {
            if( pending_timed_out )     // Uhr abgelaufen?
            {
                service_status.dwCurrentState = SERVICE_PAUSED;
            }
            else
            if( !pending_watchdog_signal )
            {
                Thread_id thread_id;
                pending_watchdog_signal = CreateEvent( NULL, FALSE, FALSE, "" );
                _beginthreadex( NULL, 0, pending_watchdog_thread, NULL, 0, &thread_id );   // Uhr aufziehen
            }
        }
        else
        {
            pending_timed_out = false;
            if( pending_watchdog_signal )  SetEvent( pending_watchdog_signal );
        }


        current_state = service_status.dwCurrentState;
    }

    LOG( "SetServiceStatus " << state_name(service_status.dwCurrentState)  << '\n' );
    BOOL ok = SetServiceStatus( service_status_handle, &service_status );
    if( !ok )
    {
        try { throw_mswin_error( "SetServiceStatus", state_name(service_status.dwCurrentState).c_str() ); }
        catch( const exception& x ) { SHOW_MSG(x.what()); }       // Was tun?
    }

    if( service_status.dwCurrentState == SERVICE_STOPPED )
    {
        service_status_handle = NULL;  // Sonst beim nächsten Mal: Fehler MSWIN-00000006  Das Handle ist ungültig. [SetServiceStatus] [SERVICE_STOPPED]
        if( service_stop )  start_self_destruction();
    }
}

//--------------------------------------------------------------------------pending_watchdog_thread

static uint __stdcall pending_watchdog_thread( void* )
{
    LOG( "pending_watchdog_thread (Überwachung des Zustands SERVICE_START_PENDING) startet\n" );

    int64 wait_until = elapsed_msec() + pending_timeout*1000;

    int state = current_state;

    if( state == SERVICE_START_PENDING )
    {
        while(1)
        {
            int wait_time = (int)( wait_until - elapsed_msec() );
            if( wait_time <= 0 )  break;

            int ret = WaitForSingleObject( pending_watchdog_signal, wait_time );
            if( ret != WAIT_TIMEOUT )  break;
        }

        THREAD_LOCK( set_service_lock )
        {
            if( current_state == state )   
            {
                LOG( "Weil der Dienst zu lange im Zustand " + state_name(state) + " ist, wird der Dienst-Zustand geändert\n" );
                pending_timed_out = true;
                set_service_status( 0, current_state );
            }

            pending_watchdog_signal.close();
        }
    }

    LOG( "pending_watchdog_thread endet\n" );

    return 0;
}

//--------------------------------------------------------------------------self_destruction_thread

static uint __stdcall self_destruction_thread( void* )
{
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    LOG( "Selbstzerstörung in " << stop_timeout << " Sekunden (ohne weitere Ankündigung) ...\n" );

    sos_sleep( stop_timeout );      // exit() sollte diesen Thread abbrechen. 25.11.2002
    {
        terminate_immediately = true;

/*      Lieber kein Log, denn das hat eine Sempahore.
        try 
        { 
            LOG( "... Selbstzerstörung. Weil der Scheduler nicht zum Ende kommt, wird der Prozess jetzt abgebrochen\n" ); 

            //BOOL ok = TerminateThread( thread_handle, 999 );
            //if( ok )  sos_sleep( 3 );
            LOG( "TerminateProcess()\n" ); 
        } 
        catch( ... ) {}
*/
        try
        {
            if( spooler_ptr )  spooler_ptr->abort_now();
        }
        catch( ... ) {}

        TerminateProcess( GetCurrentProcess(), 1 );
    }

    self_destruction_thread_id = 0;
    return 0;
}

//---------------------------------------------------------------------------start_self_destruction

static void start_self_destruction()
{
    if( !self_destruction_thread_id )  
    {
        _beginthreadex( NULL, 0, self_destruction_thread, NULL, 0, &self_destruction_thread_id );

/* Erst ab Windows 2000:
        HANDLE h = OpenThread( THREAD_QUERY_INFORMATION, false, self_destruction_thread_id  );
        SetThreadPriority( h, THREAD_PRIORITY_HIGHEST );
        SwitchToThread();
        CloseThread( h );
*/
    }
}

//----------------------------------------------------------------------string_from_handler_control

string string_from_handler_control( DWORD c )
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

//------------------------------------------------------------------------------------------Handler

static void __stdcall Handler( DWORD dwControl )
{
    LOGI( "\nService Handler(" << string_from_handler_control(dwControl) << ")\n" )

    if( spooler_ptr )
    {
        if( dwControl == SERVICE_CONTROL_STOP 
         || dwControl == SERVICE_CONTROL_SHUTDOWN )  start_self_destruction();      // Vorsichtshalber vor info()!

        spooler_ptr->log()->info( "Service Handler " + string_from_handler_control(dwControl) );

        switch( dwControl )
        {
            case SERVICE_CONTROL_STOP:              // Requests the service to stop.  
            {
                pending_timed_out = false;
                service_stop = true;
                spooler_ptr->cmd_terminate( terminate_timeout );
                set_service_status( 0, SERVICE_STOP_PENDING );

                break;
            }

            case SERVICE_CONTROL_PAUSE:             // Requests the service to pause.  
                pending_timed_out = false;
                service_stop = false;
                spooler_ptr->cmd_pause();
                break;

            case SERVICE_CONTROL_CONTINUE:          // Requests the paused service to resume.  
                pending_timed_out = false;
                service_stop = false;
                spooler_ptr->cmd_continue();
                break;

            case SERVICE_CONTROL_INTERROGATE:       // Requests the service to update immediately its current status information to the service control manager.  
                break;

            case SERVICE_CONTROL_SHUTDOWN:          // Requests the service to perform cleanup tasks, because the system is shutting down. 
                pending_timed_out = false;
                spooler_ptr->cmd_terminate();
                set_service_status( 0, SERVICE_STOP_PENDING );
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


    //set_service_status( 0 );
}

//-------------------------------------------------------------------------------------------------

static void spooler_state_changed( Spooler*, void* )
{
    set_service_status( 0 );
}

//-----------------------------------------------------------------------------------service_thread

static uint __stdcall service_thread( void* param )
{
    LOGI( "service_thread\n" );

    Ole_initialize ole;

    Service_thread_param* p   = (Service_thread_param*)param;
    int                   ret = 0;


    while(1)
    {
        Spooler spooler;

        spooler._is_service = true;
        spooler.set_state_changed_handler( spooler_state_changed );
        set_service_status( 0 );

        try
        {
            LOG( "Scheduler launch\n" );

            ret = spooler_ptr->launch( p->_argc, p->_argv, "" );

            if( spooler._shutdown_cmd == Spooler::sc_reload 
             || spooler._shutdown_cmd == Spooler::sc_load_config )  continue;        // Dasselbe in spooler.cxx, spooler_main()!
        }
        catch( const exception& x )
        {
            start_self_destruction();

            set_service_status( 0, SERVICE_PAUSED );     // Das schaltet die Diensteknöpfe frei, falls der Spooler beim eMail-Versand hängt.
            event_log( x, p->_argc, p->_argv, &spooler );
          //set_service_status( 2 );
            spooler._log.error( x.what() );
            ret = 99;
        }

        spooler_ptr = NULL;
        break;
    }

    set_service_status( 0 );       // Das beendet den Prozess wegen spooler_ptr == NULL  ==>  SERVICE_STOPPED
    LOG( "service_thread ok\n" );

    return ret;
}

//--------------------------------------------------------------------------------------ServiceMain

static void __stdcall ServiceMain( DWORD argc, char** argv )
{
    THREAD_LOCK( ServiceMain_lock )
    {
        LOGI( "ServiceMain(argc=" << argc << ")\n" );
    
        try
        {
            Thread_id               thread_id;
            DWORD                   exit_code = 0;
            Service_thread_param    param;
            int                     ret;

            for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
            {
                if( opt.with_value( "log"              ) )  log_start( opt.value() );
            }

            if( argc > 1 )  param._argc = argc, param._argv = argv;                 // Parameter des Dienstes (die sind wohl nur zum Test)
                      else  param._argc = process_argc, param._argv = process_argv; // Parameter des Programmaufrufs

            LOG( "RegisterServiceCtrlHandler\n" );
            service_status_handle = RegisterServiceCtrlHandler( spooler_service_name.c_str(), &Handler );
            if( !service_status_handle )  throw_mswin_error( "RegisterServiceCtrlHandler" );

            LOG( "CreateThread\n" );
            thread_handle.set_handle( (HANDLE)_beginthreadex( NULL, 0, service_thread, &param, 0, &thread_id ) );
            if( !thread_handle )  throw_mswin_error( "CreateThread" );

            while(1)
            {
                LOG( "MsgWaitForMultipleObjects()\n" );
                ret = MsgWaitForMultipleObjects( 1, &thread_handle._handle, FALSE, INT_MAX, QS_ALLINPUT ); 
            
                if( ret == WAIT_TIMEOUT )  continue;
                else
                if( ret == WAIT_OBJECT_0 + 1 )  windows_message_step();
                else
                    break;
            }

            if( terminate_immediately )  { LOG( "TerminateProcess()\n" ); TerminateProcess( GetCurrentProcess(), 1 ); }   // Gesetzt vom self_destruction_thread

            TerminateThread( thread_handle, 999 );   // Sollte nicht nötig sein. Nützt auch nicht, weil Destruktoren nicht gerufen werden und Komnunikations-Thread vielleicht noch läuft.
        
            LOG( "ServiceMain ok\n" );
        }
        catch( const exception& x ) { event_log( x, argc, argv ); }
    }
}

//----------------------------------------------------------------------------------spooler_service

int spooler_service( const string& service_name, int argc, char** argv )
{
    LOGI( "spooler_service(argc=" << argc << ")\n" );

    process_argc = argc;
    process_argv = argv;

    try 
    {                          
        spooler_service_name = service_name;
        SERVICE_TABLE_ENTRY ste[2];

        memset( ste, 0, sizeof ste );

        ste[0].lpServiceName = (char*)spooler_service_name.c_str();
        ste[0].lpServiceProc = ServiceMain;

        LOGI( "StartServiceCtrlDispatcher(" << ste[0].lpServiceName << ")\n" );

        BOOL ok = StartServiceCtrlDispatcher( ste );
        if( !ok )  throw_mswin_error( "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.

        LOG( "StartServiceCtrlDispatcher() OK\n" );

        THREAD_LOCK( ServiceMain_lock ) {}      // Warten, bis Thread ServiceMain sich beendet hat, erst dann diesen Mainthread beenden (sonst wird ~Sos_static zu früh gerufen)

        spooler_service_name = "";
    }
    catch( const exception& x )
    {
        event_log( x, argc, argv );
        return 1;                                                                               
    }

    LOG( "spooler_service() fertig\n" );
    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif //SYSTEM_WIN
