// $Id: spooler_service.cxx,v 1.1 2001/01/13 18:43:59 jz Exp $

#include "../kram/sos.h"
#include "../kram/log.h"
#include "spooler.h"

#ifdef SYSTEM_WIN 

#include <windows.h>

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------Windows 2000 Service

static const char               service_name[] = "Spooler";  // Windows 2000 Service
static Spooler*                 spooler_ptr;
SERVICE_STATUS_HANDLE           service_status_handle;

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
                                                                                            : SERVICE_STOPPED; 

    service_status.dwControlsAccepted           = SERVICE_ACCEPT_STOP 
                                                | SERVICE_ACCEPT_PAUSE_CONTINUE 
                                                | SERVICE_ACCEPT_SHUTDOWN 
                                                | SERVICE_ACCEPT_PARAMCHANGE; 
    
    service_status.dwWin32ExitCode              = spooler_error? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR;              
    service_status.dwServiceSpecificExitCode    = spooler_error; 
    service_status.dwCheckPoint                 = 0; 
    service_status.dwWaitHint                   = service_status.dwCurrentState == SERVICE_START_PENDING? 10*1000 : 0;  // 10s erlauben für den Start (wenn es ein Startskript mit DB-Verbindung gibt)

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
                spooler_ptr->cmd_stop();
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

            case SERVICE_CONTROL_NETBINDADD:        // Windows 2000: Notifies a network service that there is a new component for binding. The service should bind to the new component.  
                break;

            case SERVICE_CONTROL_NETBINDREMOVE:     // Windows 2000: Notifies a network service that a component for binding has been removed. The service should reread its binding information and unbind from the removed component.  
                break;

            case SERVICE_CONTROL_NETBINDENABLE:     // Windows 2000: Notifies a network service that a disabled binding has been enabled. The service should reread its binding information and add the new binding.  
                break;

            case SERVICE_CONTROL_NETBINDDISABLE:    // Windows 2000: Notifies a network service that one of its bindings has been disabled. The service should reread its binding information and remove the binding. 
                break;

            default:
                break;
        }
    }


    set_service_status();
}

//--------------------------------------------------------------------------------------ServiceMain

static void __stdcall ServiceMain( DWORD argc, char** argv )
{
    LOGI( "ServiceMain()\n" );

    Spooler spooler;

    try
    {
        spooler_ptr = &spooler;

        service_status_handle = RegisterServiceCtrlHandler( service_name, &Handler );
        if( !service_status_handle )  throw_mswin_error( "RegisterServiceCtrlHandler" );
    }
    catch( const Xc& )
    {
        set_service_status(2);
        spooler_ptr = NULL;
        throw;
    }
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

        DWORD ret = StartServiceCtrlDispatcher( ste );
        if( !ret )  throw_mswin_error( "StartServiceCtrlDispatcher" );      // Z.B. nach 15s: MSWIN-00000427  Der Dienstprozess konnte keine Verbindung zum Dienstcontroller herstellen.
    }
    catch( const Xc& x )
    {

        HANDLE h; 
        const char* what = x.what().c_str();
 
        h = RegisterEventSource( NULL, "Application" );
     
        ReportEvent( h,                     // event log handle 
                     EVENTLOG_ERROR_TYPE,   // event type 
                     0,                     // category zero 
                     1,                     // event identifier ???
                     NULL,                  // no user security identifier 
                     1,                     // one substitution string 
                     x.what().length(),     // no data 
                     &what,                 // pointer to string array 
                     (void*)x.what().c_str() );    // pointer to data 
 
        DeregisterEventSource( h ); 

        cerr << x;
        return 1;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif //SYSTEM_WIN