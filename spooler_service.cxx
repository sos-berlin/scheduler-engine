// $Id: spooler_service.cxx,v 1.16 2002/04/06 20:07:40 jz Exp $
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
#include "../kram/sleep.h"

#ifdef SYSTEM_WIN 

#include <windows.h>

namespace sos {

string module_filename();

namespace spooler {

//--------------------------------------------------------------------------------------------const

static const char               std_service_name[]          = "DocumentFactory_Spooler";    // Windows 2000 Service
static const char               std_service_display_name[]  = "DocumentFactory Spooler";    // Gezeigter Name
static const char               service_description[]       = "Hintergrund-Jobs der Document Factory";
static const int                stop_timeout                = 30;

static bool                     terminate_immediately       = false;
static bool                     service_stop                = false;                        // STOP-Kommando von der Dienstesteuerung (und nicht von TCP-Schnittstelle)

//-----------------------------------------------------------------------------------Service_handle

struct Service_handle
{
                                Service_handle              ( SC_HANDLE h = NULL )          : _handle(h) {}
                               ~Service_handle              ()                              { close(); }

        void                    operator =                  ( SC_HANDLE h )                 { set_handle( h ); }
                                operator HANDLE             () const                        { return _handle; }
                                operator !                  () const                        { return _handle == 0; }

        void                    set_handle                  ( SC_HANDLE h )                 { close(); _handle = h; }
        SC_HANDLE               handle                      () const                        { return _handle; }
        void                    close                       ()                              { if(_handle) { CloseServiceHandle(_handle); _handle=NULL; } }

        SC_HANDLE              _handle;

  private:
                                Service_handle              ( const Service_handle& );      // Nicht implementiert
    void                        operator =                  ( const Service_handle& );      // Nicht implementiert
};

//-------------------------------------------------------------------------------------------static

static Spooler*                 spooler_ptr;                // Wird auch beim Zwangsbeenden auf NULL gesetzt
SERVICE_STATUS_HANDLE           service_status_handle;
Handle                          thread_handle;
string                          spooler_service_name;
int                             process_argc;
char**                          process_argv;

//-----------------------------------------------------------------------------Service_thread_param

struct Service_thread_param
{
    int                        _argc;
    char**                     _argv;
};

//-------------------------------------------------------------------------------------service_name

static string service_name( const string& id )
{
    if( id.empty() )  return std_service_name;
                else  return std_service_name + ( "_" + id );
}

//----------------------------------------------------------------------------------------event_log

static void event_log( const string& msg_par )
{
    string msg = "***DOCUMENTFACTORY SPOOLER*** " + msg_par;

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

void install_service( const string& id, const string& params ) 
{ 
    string my_service_name         = service_name(id);
    string my_service_display_name = std_service_display_name;

    if( !id.empty() )  my_service_display_name += " -id=" + id;

    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );


    string command_line = module_filename();
    if( command_line.find(" ") != string::npos )  command_line = quoted_string( command_line, '"', '"' );
    if( !params.empty() )  command_line += " " + params;

    SC_HANDLE service_handle = CreateService( 
                                    manager_handle,            // SCManager database 
                                    my_service_name.c_str(),      // name of service 
                                    my_service_display_name.c_str(),// service name to display 
                                    SERVICE_ALL_ACCESS,        // desired access 
                                    SERVICE_WIN32_OWN_PROCESS, // service type 
                                    SERVICE_DEMAND_START,      // start type   oder SERVICE_AUTO_START
                                    SERVICE_ERROR_NORMAL,      // error control type 
                                    command_line.c_str(),      // service's binary 
                                    NULL,                      // no load ordering group 
                                    NULL,                      // no tag identifier 
                                    NULL,                      // no dependencies 
                                    NULL,                      // LocalSystem account 
                                    NULL );                    // no password 

    if( !service_handle )  throw_mswin_error( "CreateService" );


    OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
    GetVersionEx( &v );
    if( v.dwMajorVersion >= 5 )     // Windows 2000?
    {
        string descr = service_description;
        if( !params.empty() )  descr += " " + params;
        SERVICE_DESCRIPTION d;
        d.lpDescription = (char*)descr.c_str();
        ChangeServiceConfig2( service_handle, SERVICE_CONFIG_DESCRIPTION, &d );
    }


    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//-----------------------------------------------------------------------------------remove_service

void remove_service( const string& id ) 
{ 
    BOOL ok;

    SC_HANDLE manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );

    string my_service_name = service_name(id);
    SC_HANDLE service_handle = OpenService( manager_handle, my_service_name.c_str(), DELETE );
    if( !service_handle )  throw_mswin_error( "OpenService" );

    ok = DeleteService( service_handle );
    if( !ok )  throw_mswin_error( "DeleteService" );

    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );
} 

//------------------------------------------------------------------------------------service_state

DWORD service_state( const string& id )
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
        string         my_service_name = service_name(id);
        SERVICE_STATUS status;  memset( &status, 0, sizeof status );

        manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );
        if( !manager_handle ) {
            if( GetLastError() == ERROR_ACCESS_DENIED          )  goto ENDE;
            throw_mswin_error( "OpenSCManager" );
        }

        service_handle = OpenService(manager_handle, my_service_name.c_str(), SERVICE_QUERY_STATUS );
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
    catch( const Xc& )
    {
        CloseServiceHandle( service_handle ); 
        CloseServiceHandle( manager_handle );
    }

    return result;
}

//------------------------------------------------------------------------------------service_start

void service_start( const string& id )
{
    BOOL            ok;
    Service_handle  manager_handle;
    Service_handle  service_handle;
    string          my_service_name = service_name(id);

    manager_handle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE );    if( !manager_handle )  throw_mswin_error( "OpenSCManager" );
    service_handle = OpenService( manager_handle, my_service_name.c_str(), SERVICE_START );         if( !service_handle )  throw_mswin_error( "OpenService" );
    ok = StartService( service_handle, 0, NULL );                                                   if( !ok )  throw_mswin_error( "StartService" );
}

//----------------------------------------------------------------------------------service_handler

static void set_service_status( int spooler_error, int state = 0 )
{
    if( !service_status_handle )  return;

    SERVICE_STATUS service_status;

    DWORD stop_pending = service_stop? SERVICE_STOP_PENDING     // Nur, wenn Dienstesteuerung den Spooler beendet (nicht, wenn ein TCP-Kdo beendet)
                                     : SERVICE_RUNNING;

    service_status.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;

    service_status.dwCurrentState               = !spooler_ptr? SERVICE_STOPPED 
                                                : spooler_ptr->state() == Spooler::s_stopped         ? SERVICE_PAUSED //SetServiceStatus() ruft exit()!
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
    service_status.dwWaitHint                   = service_status.dwCurrentState == SERVICE_START_PENDING? 10*1000 : 0;  // 10s erlauben für den Start (wenn es ein Startskript mit DB-Verbindung gibt)
    service_status.dwWaitHint                   = service_status.dwCurrentState == SERVICE_STOP_PENDING? stop_timeout*1000 : 0;  // 10s erlauben für den Start (wenn es ein Startskript mit DB-Verbindung gibt)

    string state_name;
    switch( service_status.dwCurrentState )  
    {
        case SERVICE_STOPPED        : state_name = "SERVICE_STOPPED";       break;
        case SERVICE_PAUSED         : state_name = "SERVICE_PAUSED";        break;
        case SERVICE_START_PENDING  : state_name = "SERVICE_START_PENDING"; break;
        case SERVICE_STOP_PENDING   : state_name = "SERVICE_STOP_PENDING";  break;
        case SERVICE_RUNNING        : state_name = "SERVICE_RUNNING";       break;
        default                     : state_name = as_string( (int)service_status.dwCurrentState );
    }

    LOG( "SetServiceStatus " << state_name << '\n' );
    BOOL ok = SetServiceStatus( service_status_handle, &service_status );
    if( !ok )
    {
        try { throw_mswin_error( "SetServiceStatus", state_name.c_str() ); }
        catch( const Xc& x ) { SHOW_MSG(x); }       // Was tun?
    }
    
    // Keine Wiederkehr bei SERVICE_STOPPED
}

//--------------------------------------------------------------------------------------kill_thread

static uint __stdcall kill_thread( void* param )
{
    double timeout = stop_timeout;

    while( timeout > 0 )
    {
        double wait = 0.1;
        sos_sleep( wait );

        if( !spooler_is_running )  return 0;

        timeout -= wait;
    }

    LOG( "Weil der Spooler nicht zum Ende kommt, wird jetzt der Prozess sofort beendet\n" );
    terminate_immediately = true;
    BOOL ok = TerminateThread( thread_handle, 999 );

    if( ok )  sos_sleep( 3 );

    LOG( "TerminateProcess()\n" ); 
    TerminateProcess(GetCurrentProcess(),1);
    return 0;
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
            {
                service_stop = true;
                spooler_ptr->cmd_terminate();
                set_service_status( 0, SERVICE_STOP_PENDING );

                Thread_id thread_id;

                _beginthreadex( NULL, 0, kill_thread, NULL, 0, &thread_id );

                break;
            }

            case SERVICE_CONTROL_PAUSE:             // Requests the service to pause.  
                service_stop = false;
                spooler_ptr->cmd_pause();
                break;

            case SERVICE_CONTROL_CONTINUE:          // Requests the paused service to resume.  
                service_stop = false;
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


    set_service_status( 0 );
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

    {
        Spooler spooler;
        spooler_ptr = &spooler;

        spooler._is_service = true;
        spooler.set_state_changed_handler( spooler_state_changed );
        set_service_status( 0 );

        try
        {
            LOG( "Spooler launch\n" );
            ret = spooler_ptr->launch( p->_argc, p->_argv );

            // Soweit kommt's nicht, denn SetServiceStatus(STOPPED) beendet den Prozess mit exit()
        }
        catch( const Xc& x )
        {
            event_log( x.what() );
            set_service_status( 2 );
            spooler._log.error( x.what() );
            spooler_ptr = NULL;
            ret = 99;
          //throw;
        }

        spooler_ptr = NULL;
    }

    set_service_status( 0 );       // Das beendet den Prozess wegen spooler_ptr == NULL  ==>  SERVICE_STOPPED
    LOG( "service_thread ok\n" );

    return ret;
}

//--------------------------------------------------------------------------------------ServiceMain

static void __stdcall ServiceMain( DWORD argc, char** argv )
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
        thread_handle = _beginthreadex( NULL, 0, service_thread, &param, 0, &thread_id );
        if( !thread_handle )  throw_mswin_error( "CreateThread" );

        //do ret = WaitForSingleObject( thread_handle, INT_MAX );  while( ret != WAIT_TIMEOUT );
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

        if( terminate_immediately )  { LOG( "_exit(1);\n" ); _exit(0); }

        TerminateThread( thread_handle, 999 );   // Sollte nicht nötig sein. Nützt auch nicht, weil Destruktoren nicht gerufen werden und Komnunikations-Thread vielleicht noch läuft.
        
        //GetExitCodeThread( thread_handle, &exit_code );
        //if( exit_code == 1 )
        //{
        //    restart_service( argc, argv );
        //}
        LOG( "ServiceMain ok\n" );
    }
    catch( const Xc& x        ) { event_log( x.what() ); }
    catch( const exception& x ) { event_log( x.what() ); }
}

//----------------------------------------------------------------------------------spooler_service

int spooler_service( const string& id, int argc, char** argv )
{
    LOGI( "spooler_service(argc=" << argc << ")\n" );

    process_argc = argc;
    process_argv = argv;

    try 
    {                          
        spooler_service_name = service_name(id);
        SERVICE_TABLE_ENTRY ste[2];

        memset( ste, 0, sizeof ste );

        ste[0].lpServiceName = (char*)spooler_service_name.c_str();
        ste[0].lpServiceProc = ServiceMain;

        LOGI( "StartServiceCtrlDispatcher(" << ste[0].lpServiceName << ")\n" );

        BOOL ok = StartServiceCtrlDispatcher( ste );
        if( !ok )  throw_mswin_error( "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.

        spooler_service_name = "";
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