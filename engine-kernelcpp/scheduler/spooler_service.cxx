// $Id: spooler_service.cxx 13786 2009-04-28 08:03:45Z jz $
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
namespace scheduler {

using namespace zschimmer::windows;

//--------------------------------------------------------------------------------------------const

static const char               std_service_name[]          = "sos_scheduler";    // Windows 2000 Service
static const char               std_service_display_name[]  = "SOS Scheduler";    // Gezeigter Name
static const int                terminate_timeout           = 30;
static const int                stop_timeout                = terminate_timeout + 5;        // Zeit lassen, um Datenbank zu schließen, dann abort_now() per Thread
static const int                pending_timeout             = 15;

static Spooler* static_service_spooler = NULL;

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

//-----------------------------------------------------------------------------My_scheduler_service

struct My_scheduler_service {
    Fill_zero                  _zero_;
    bool                       _terminate_immediately;
    bool                       _service_stop;                                               // STOP-Kommando von der Dienstesteuerung (und nicht von TCP-Schnittstelle)
    Thread_id                  _self_destruction_thread_id;

    SERVICE_STATUS_HANDLE      _service_status_handle;
    Handle                     _thread_handle;
    string                     _spooler_service_name;
    int                        _process_argc;
    char**                     _process_argv;

    // Zustände SERVICE_START_PENDING und SERVICE_STOP_PENDING nicht länger als pending_timeout Sekunden:
    Thread_semaphore           _set_service_lock;
    Thread_semaphore           _ServiceMain_lock;
    Handle                     _pending_watchdog_signal;
    bool                       _pending_timed_out;
    int                        _current_state;

    My_scheduler_service() : 
        _zero_(this+1),
        _set_service_lock("set_service"),
        _ServiceMain_lock("ServiceMain")
    {}

    int                         spooler_service             ( const string& service_name, int argc, char** argv );
    void                        set_service_status          ( int spooler_error, int state = 0 );
    uint                        pending_watchdog_thread     (void*);
    uint                        self_destruction_thread     (void*);
    DWORD                       handle                      ( DWORD dwControl, DWORD event, void* event_data, void* );
    uint                        service_thread              (void*);
    void                        service_main                (DWORD argc, char** argv);
};

static My_scheduler_service* scheduler_service;    // Wird nicht freigeben. Destruktor braucht nicht aufgerufen zu werden, Prozess wird eh beendet.

//-------------------------------------------------------------------------------------------------

static void                     start_self_destruction      ();
static uint __stdcall           call_pending_watchdog_thread( void* );

//-----------------------------------------------------------------------------Service_thread_param

struct Service_thread_param
{
    int                        _argc;
    char**                     _argv;
};

//---------------------------------------------------------------------------------make_service_name

string make_service_name( const string& id, bool is_backup )
{
    S result;

    result << std_service_name;

    if( !id.empty() )  result << "_" << id;
    if( is_backup )  result << "_backup";

    return result;
}

//------------------------------------------------------------------------------make_service_display

string make_service_display( const string& id, bool is_backup )
{
    S result;

    result << std_service_display_name;
    if( !id.empty() )  result << " -id=" << id;
    if( is_backup   )  result << " -backup";

    return result;
}

//----------------------------------------------------------------------------------------event_log

static void event_log( const exception& x, int argc, char** argv, Spooler* spooler = NULL )
{
    string what = x.what();
    Z_LOG2( "scheduler.service", "event_log() ERROR  " << what << "\n" );

    string msg = "*** SOS SCHEDULER *** " + what;

    HANDLE h = RegisterEventSource( NULL, "Application" );
    const char* m = msg.c_str();
 
    Z_LOG2( "scheduler.service", "ReportEvent()\n" );
    ReportEvent( h,                     // event log handle 
                 EVENTLOG_ERROR_TYPE,   // event type 
                 0,                     // category zero 
                 1,                     // event identifier ???
                 NULL,                  // no user security identifier 
                 1,                     // one substitution string 
                 int_cast(msg.length()),// no data 
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


    string command_line = quoted_windows_process_parameter( program_filename() );
    if( !params.empty() )  command_line += " " + params;

    Z_LOG2( "scheduler.service", "CreateService(,\"" << service_name << "\", \"" << service_display << "\",,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,"
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
        Z_LOG2( "scheduler.service", "ChangeServiceConfig2(,SERVICE_CONFIG_DESCRIPTION,\"" << service_description << "\")\n" );
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

    Z_LOG2( "scheduler.service", "DeleteService(\"" << service_name << "\")\n" );

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
    Service_handle manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );
    
    Service_handle service_handle = OpenService( manager_handle, service_name.c_str(), SERVICE_START );  
    if( !service_handle )  throw_mswin_error( "OpenService" );

    Z_LOG2( "scheduler.service", "ServiceStart(\"" << service_name << "\")\n" );
    BOOL ok = StartService( service_handle, 0, NULL );
    if( !ok )  throw_mswin_error( "StartService" );
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

void My_scheduler_service::set_service_status( int spooler_error, int state )
{
    SERVICE_STATUS  service_status;
    string          state_nam;

    if( !_service_status_handle )  return;

    Z_MUTEX( _set_service_lock )
    {
        DWORD stop_pending = _service_stop? SERVICE_STOP_PENDING    // Nur, wenn Dienstesteuerung den Spooler beendet 
                                          : SERVICE_RUNNING;        // Wenn Spooler über TCP beendet wird, soll der Diensteknopf "beenden" frei bleiben. Deshalb paused.

        service_status.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;

        service_status.dwCurrentState               = state                                              ? state
                                                    : !static_service_spooler                                       ? SERVICE_STOPPED 
                                                    : static_service_spooler->state() == Spooler::s_stopped         ? SERVICE_STOPPED       //SetServiceStatus() ruft exit()!
                                                    : static_service_spooler->state() == Spooler::s_starting        ? SERVICE_START_PENDING
                                                    : static_service_spooler->state() == Spooler::s_waiting_for_activation? SERVICE_RUNNING       // START_PENDING blockiert Bedienknöpfe der Dienstesteuerung (Windows XP)
                                                    : static_service_spooler->state() == Spooler::s_waiting_for_activation_paused? SERVICE_RUNNING       // START_PENDING blockiert Bedienknöpfe der Dienstesteuerung (Windows XP)
                                                    : static_service_spooler->state() == Spooler::s_stopping        ? stop_pending
                                                    : static_service_spooler->state() == Spooler::s_stopping_let_run? stop_pending
                                                    : static_service_spooler->state() == Spooler::s_running         ? SERVICE_RUNNING
                                                    : static_service_spooler->state() == Spooler::s_paused          ? SERVICE_PAUSED
                                                                                                         : SERVICE_START_PENDING; 
        service_status.dwControlsAccepted           = SERVICE_ACCEPT_STOP 
                                                    | SERVICE_ACCEPT_PAUSE_CONTINUE 
                                                    | SERVICE_ACCEPT_SHUTDOWN
                                                    | SERVICE_ACCEPT_POWEREVENT;
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
            if( _pending_timed_out )     // Uhr abgelaufen?
            {
                service_status.dwCurrentState = SERVICE_PAUSED;
            }
            else
            if( !_pending_watchdog_signal )
            {
                Thread_id thread_id;
                _pending_watchdog_signal = convert_to_noninheritable_handle( CreateEvent( NULL, FALSE, FALSE, "" ) );
                _beginthreadex( NULL, 0, call_pending_watchdog_thread, NULL, 0, &thread_id );   // Uhr aufziehen
            }
        }
        else
        {
            _pending_timed_out = false;
            if( _pending_watchdog_signal )  SetEvent( _pending_watchdog_signal );
        }


        _current_state = service_status.dwCurrentState;
    }

    Z_LOG2( "scheduler.service", "SetServiceStatus " << state_name(service_status.dwCurrentState)  << '\n' );
    BOOL ok = SetServiceStatus( _service_status_handle, &service_status );
    if( !ok )
    {
        try { throw_mswin_error( "SetServiceStatus", state_name(service_status.dwCurrentState).c_str() ); }
        catch( const exception& x ) { SHOW_MSG(x.what()); }       // Was tun?
    }

    if( service_status.dwCurrentState == SERVICE_STOPPED )
    {
        _service_status_handle = NULL;  // Sonst beim nächsten Mal: Fehler MSWIN-00000006  Das Handle ist ungültig. [SetServiceStatus] [SERVICE_STOPPED]
        if( _service_stop )  start_self_destruction();
    }
}

//---------------------------------------------------------------------call_pending_watchdog_thread

static uint __stdcall call_pending_watchdog_thread(void* o)
{
    Z_LOG2( "scheduler.service", "pending_watchdog_thread (watching state SERVICE_START_PENDING) starts\n" );
    return scheduler_service->pending_watchdog_thread(o);
}

//----------------------------------------------------My_scheduler_service::pending_watchdog_thread

uint My_scheduler_service::pending_watchdog_thread( void* ) 
{
    int64 wait_until = elapsed_msec() + pending_timeout*1000;

    int state = _current_state;

    if( state == SERVICE_START_PENDING )
    {
        while(1)
        {
            int wait_time = (int)( wait_until - elapsed_msec() );
            if( wait_time <= 0 )  break;

            int ret = WaitForSingleObject( _pending_watchdog_signal, wait_time );
            if( ret != WAIT_TIMEOUT )  break;
        }

        Z_MUTEX( _set_service_lock )
        {
            if( _current_state == state )   
            {
                Z_LOG2( "scheduler.service", "Service has been too long in state '" + state_name(state) + "' so state is being changed\n" );
                _pending_timed_out = true;
                set_service_status( 0, _current_state );
            }

            _pending_watchdog_signal.close();
        }
    }

    Z_LOG2( "scheduler.service", "pending_watchdog_thread terminates\n" );

    return 0;
}

//--------------------------------------------------------------------------self_destruction_thread

static uint __stdcall call_self_destruction_thread(void* o)
{
    return scheduler_service->self_destruction_thread(o);
}

//----------------------------------------------------My_scheduler_service::self_destruction_thread

uint My_scheduler_service::self_destruction_thread(void*) {
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    Z_LOG2( "scheduler.service", "Self destruction in " << stop_timeout << " seconds, without further notice ...\n" );

    sos_sleep( stop_timeout );      // exit() sollte diesen Thread abbrechen. 25.11.2002
    {
        _terminate_immediately = true;

/*      Lieber kein Log, denn das hat eine Sempahore.
        try 
        { 
            Z_LOG2( "scheduler.service", "... Selbstzerstörung. Weil der Scheduler nicht zum Ende kommt, wird der Prozess jetzt abgebrochen\n" ); 

            //BOOL ok = TerminateThread( _thread_handle, 999 );
            //if( ok )  sos_sleep( 3 );
            Z_LOG2( "scheduler.service", "TerminateProcess()\n" ); 
        } 
        catch( ... ) {}
*/
        try
        {
            if( static_service_spooler )  static_service_spooler->abort_now();
        }
        catch( ... ) {}

        TerminateProcess( GetCurrentProcess(), 1 );
    }

    _self_destruction_thread_id = 0;
    return 0;
}

//---------------------------------------------------------------------------start_self_destruction

static void start_self_destruction()
{
    if( !scheduler_service->_self_destruction_thread_id )  
    {
        _beginthreadex( NULL, 0, call_self_destruction_thread, NULL, 0, &scheduler_service->_self_destruction_thread_id );

/* Erst ab Windows 2000:
        HANDLE h = OpenThread( THREAD_QUERY_INFORMATION, false, _self_destruction_thread_id  );
        SetThreadPriority( h, THREAD_PRIORITY_HIGHEST );
        SwitchToThread();
        CloseThread( h );
*/
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
        case SERVICE_CONTROL_POWEREVENT:        return "SERVICE_CONTROL_POWEREVENT";
        default:                                return "SERVICE_CONTROL_" + as_string( c );
    }
}

//--------------------------------------------------------------------------string_from_power_event

static string string_from_power_event( DWORD e )
{
    switch( e )
    {
        case PBT_APMBATTERYLOW:         return "PBT_APMBATTERYLOW";
        case PBT_APMOEMEVENT:           return "PBT_APMOEMEVENT";
        case PBT_APMPOWERSTATUSCHANGE:  return "PBT_APMPOWERSTATUSCHANGE";
        case PBT_APMQUERYSUSPEND:       return "PBT_APMQUERYSUSPEND";
        case PBT_APMQUERYSUSPENDFAILED: return "PBT_APMQUERYSUSPENDFAILED";
        case PBT_APMSUSPEND:            return "PBT_APMSUSPEND";
        case PBT_APMRESUMEAUTOMATIC:    return "PBT_APMRESUMEAUTOMATIC";
        case PBT_APMRESUMECRITICAL:     return "PBT_APMRESUMECRITICAL";
        case PBT_APMRESUMESUSPEND:      return "PBT_APMRESUMESUSPEND";
        default:                        return "PBT_" + as_string( e );
    }
}

//----------------------------------------------------------------------------------------HandlerEx

static DWORD WINAPI HandlerEx( DWORD dwControl, DWORD event, void* event_data, void* p)
{
    return scheduler_service->handle(dwControl, event, event_data, p);
}

//---------------------------------------------------------------------My_scheduler_service::handle

DWORD My_scheduler_service::handle( DWORD dwControl, DWORD event, void* event_data, void* )
{
    Z_LOGI2( "scheduler.service", "Service HandlerEx(" << string_from_handler_control(dwControl) << "," << 
             ( dwControl == SERVICE_CONTROL_POWEREVENT? string_from_power_event( event ) : as_string( event ) ) << "," <<
             event_data << ")\n" );

    DWORD result = ERROR_CALL_NOT_IMPLEMENTED;

    if( static_service_spooler )
    {
        if( dwControl == SERVICE_CONTROL_STOP 
         || dwControl == SERVICE_CONTROL_SHUTDOWN )  start_self_destruction();      // Vorsichtshalber vor info()!

        Message_string m ( "SCHEDULER-960", string_from_handler_control(dwControl),
                           ( dwControl == SERVICE_CONTROL_POWEREVENT? string_from_power_event( event ) : as_string( event ) ) );

        Z_LOG2( "scheduler.service", m.as_string() << "\n" );

        switch( dwControl )
        {
            case SERVICE_CONTROL_STOP:              // Requests the service to stop.  
            {
                _pending_timed_out = false;
                _service_stop = true;
                static_service_spooler->cmd_terminate( false, terminate_timeout );    // Shutdown des Clusters
                set_service_status( 0, SERVICE_STOP_PENDING );
                result = NO_ERROR;  
                break;
            }

            case SERVICE_CONTROL_PAUSE:             // Requests the service to pause.  
                _pending_timed_out = false;
                _service_stop = false;
                static_service_spooler->cmd_pause();
                result = NO_ERROR;  
                break;

            case SERVICE_CONTROL_CONTINUE:          // Requests the paused service to resume.  
                _pending_timed_out = false;
                _service_stop = false;
                static_service_spooler->cmd_continue();
                result = NO_ERROR;  
                break;

            case SERVICE_CONTROL_INTERROGATE:       // Requests the service to update immediately its current status information to the service control manager.  
                result = NO_ERROR;  
                break;

            case SERVICE_CONTROL_SHUTDOWN:          // Requests the service to perform cleanup tasks, because the system is shutting down. 
                // Wir haben nicht mehr als 20s: Das ist eigentlich zu kurz. STOP_PENDING gibt eine kleine Gnadenfrist. 2006-06-19
                _pending_timed_out = false;
                static_service_spooler->cmd_terminate( false, terminate_timeout );   // Shutdown des Clusters  (nicht mehr: // Kein shutdown des Clusters, ein anderer Rechner soll übernehmen.)
                set_service_status( 0, SERVICE_STOP_PENDING );
                result = NO_ERROR;  
                break;

            case SERVICE_CONTROL_PARAMCHANGE:       // Windows 2000: Notifies the service that service-specific startup parameters have changed. The service should reread its startup parameters. 
                break;
/*
            case SERVICE_CONTROL_NETBINDADD:        // Windows 2000: Notifies a network service that there is a new component for binding. The service should bind to the new component.  
            case SERVICE_CONTROL_NETBINDREMOVE:     // Windows 2000: Notifies a network service that a component for binding has been removed. The service should reread its binding information and unbind from the removed component.  
            case SERVICE_CONTROL_NETBINDENABLE:     // Windows 2000: Notifies a network service that a disabled binding has been enabled. The service should reread its binding information and add the new binding.  
            case SERVICE_CONTROL_NETBINDDISABLE:    // Windows 2000: Notifies a network service that one of its bindings has been disabled. The service should reread its binding information and remove the binding. 
*/

            case SERVICE_CONTROL_POWEREVENT:
            {
                result = NO_ERROR;  

                // Reihenfolge:
                // 1) PBT_APMQUERYSUSPEND
                // 2) PBT_APMSUSPEND
                // 3) suspended
                // 4) PBT_APMRESUMESUSPEND und PBT_APMRESUMEAUTOMATIC (Reihenfolge vertauschbar)

                if( static_service_spooler )
                {
                    switch( event )
                    {
                        case PBT_APMQUERYSUSPEND:
                        {
                            // Das verhindert das Schlafenlegen durch den Benutzer (ist das gut? Ein Kennzeichen im System Tray wäre gut)
                            result = static_service_spooler->is_machine_suspendable()? NO_ERROR : BROADCAST_QUERY_DENY;
                            break;
                        }

                        //case PBT_APMQUERYSUSPENDFAILED:
                        //case PBT_APMBATTERYLOW:     // Vorsichtshalber?
                        //case PBT_APMSUSPEND:        // Keine Zeit mehr
                        //{
                        //    result = NO_ERROR;  
                        //    break;
                        //}

                        //case PBT_APMQUERYSUSPENDFAILED: // Suspend hat nicht geklappt, also alles wieder starten:
                        //case PBT_APMRESUMEAUTOMATIC:    // Erwachen ohne Benutzer (z.B. durch Netzwerkkarte)
                        //case PBT_APMRESUMESUSPEND:
                        //case PBT_APMRESUMECRITICAL:
                        //{
                        //    result = NO_ERROR;  
                        //    break;
                        //}

                        default: ;
                    }
                }

                break;
            }

            default:
                break;
        }
    }


    //set_service_status( 0 );
    return result;
}

//-------------------------------------------------------------------------------------------------

static void spooler_state_changed( Spooler*, void* )
{
    scheduler_service->set_service_status( 0 );
}

//-----------------------------------------------------------------------------------service_thread

static uint __stdcall call_service_thread( void* param )
{
    return scheduler_service->service_thread(param);
}

//-------------------------------------------------------------My_scheduler_service::service_thread

uint My_scheduler_service::service_thread(void* param) 
{
    Z_LOGI2( "scheduler.service", "service_thread\n" );

    Ole_initialize ole;

    Service_thread_param* p   = (Service_thread_param*)param;
    int                   ret = 0;


    while(1)
    {
        Spooler spooler;
        static_service_spooler = &spooler;

        spooler._is_service = true;
        spooler.set_state_changed_handler( spooler_state_changed );
        set_service_status( 0 );


        try
        {
            Z_LOG2( "scheduler.service", "Scheduler launch\n" );

            ret = spooler.launch( p->_argc, p->_argv, "" );

            if( spooler._shutdown_cmd == Spooler::sc_reload 
             || spooler._shutdown_cmd == Spooler::sc_load_config )  continue;        // Dasselbe in spooler.cxx, spooler_main()!
        }
        catch( const exception& x )
        {
            start_self_destruction();

            set_service_status( 0, SERVICE_PAUSED );     // Das schaltet die Diensteknöpfe frei, falls der Spooler beim eMail-Versand hängt.
            event_log( x, p->_argc, p->_argv, &spooler );
          //set_service_status( 2 );
            Z_LOG2("scheduler", x.what() );
            ret = 99;
        }

        static_service_spooler = NULL;
        break;
    }

    set_service_status( 0 );       // Das beendet den Prozess wegen static_service_spooler == NULL  ==>  SERVICE_STOPPED
    Z_LOG2( "scheduler.service", "service_thread ok\n" );

    return ret;
}

//--------------------------------------------------------------------------------------ServiceMain

static void __stdcall ServiceMain( DWORD argc, char** argv )
{
    scheduler_service->service_main(argc, argv);
}

//---------------------------------------------------------------My_scheduler_service::service_main

void My_scheduler_service::service_main(DWORD argc, char** argv) 
{
    Z_MUTEX( _ServiceMain_lock )
    {
        Z_LOGI2( "scheduler.service", "ServiceMain(argc=" << argc << ")\n" );
    
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
                      else  param._argc = _process_argc, param._argv = _process_argv; // Parameter des Programmaufrufs


            Z_LOG2( "scheduler.service", "RegisterServiceCtrlHandlerEx\n" );
            _service_status_handle = RegisterServiceCtrlHandlerEx( _spooler_service_name.c_str(), &HandlerEx, NULL );
            if( !_service_status_handle )  throw_mswin_error( "RegisterServiceCtrlHandler" );

            Z_LOG2( "scheduler.service", "CreateThread\n" );
            _thread_handle.set_handle_noninheritable( (HANDLE)_beginthreadex( NULL, 0, call_service_thread, &param, 0, &thread_id ) );
            if( !_thread_handle )  throw_mswin_error( "CreateThread" );

            while(1)
            {
                Z_LOG2( "scheduler.service", "MsgWaitForMultipleObjects()\n" );
                ret = MsgWaitForMultipleObjects( 1, &_thread_handle._handle, FALSE, INT_MAX, QS_ALLINPUT ); 
            
                if( ret == WAIT_TIMEOUT )  continue;
                else
                if( ret == WAIT_OBJECT_0 + 1 )  windows_message_step();
                else
                    break;
            }

            if( _terminate_immediately )  { Z_LOG2( "scheduler.service", "TerminateProcess()\n" ); TerminateProcess( GetCurrentProcess(), 1 ); }   // Gesetzt vom self_destruction_thread

            TerminateThread( _thread_handle, 999 );   // Sollte nicht nötig sein. Nützt auch nicht, weil Destruktoren nicht gerufen werden und Komnunikations-Thread vielleicht noch läuft.
        
            Z_LOG2( "scheduler.service", "ServiceMain ok\n" );
        }
        catch( const exception& x ) { event_log( x, argc, argv ); }
    }
}

//----------------------------------------------------------------------------------spooler_service

int spooler_service( const string& service_name, int argc, char** argv ) {
    assert(scheduler_service == NULL);
    scheduler_service = new My_scheduler_service();
    return scheduler_service->spooler_service(service_name, argc, argv);
}

//------------------------------------------------------------My_scheduler_service::spooler_service

int My_scheduler_service::spooler_service( const string& service_name, int argc, char** argv )
{
    Z_LOGI2( "scheduler.service", "spooler_service(argc=" << argc << ")\n" );

    _process_argc = argc;
    _process_argv = argv;

    try 
    {                          
        _spooler_service_name = service_name;
        SERVICE_TABLE_ENTRY ste[2];

        memset( ste, 0, sizeof ste );

        ste[0].lpServiceName = (char*)_spooler_service_name.c_str();
        ste[0].lpServiceProc = ServiceMain;

        Z_LOGI2( "scheduler.service", "StartServiceCtrlDispatcher(" << ste[0].lpServiceName << ")\n" );

        BOOL ok = StartServiceCtrlDispatcher( ste );
        if( !ok )  throw_mswin_error( "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.

        Z_LOG2( "scheduler.service", "StartServiceCtrlDispatcher() OK\n" );

        Z_MUTEX( _ServiceMain_lock ) {}      // Warten, bis Thread ServiceMain sich beendet hat, erst dann diesen Mainthread beenden (sonst wird ~Sos_static zu früh gerufen)

        _spooler_service_name = "";
    }
    catch( const exception& x )
    {
        event_log( x, argc, argv );
        return 1;                                                                               
    }

    Z_LOG2( "scheduler.service", "spooler_service() fertig\n" );
    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif //SYSTEM_WIN
