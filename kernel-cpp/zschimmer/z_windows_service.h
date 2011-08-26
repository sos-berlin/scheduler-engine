// $Id: z_windows_service.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __ZSCHIMMER_WINDOWS_SERVICE_H
#define __ZSCHIMMER_WINDOWS_SERVICE_H


#include "z_windows.h"
#include "threads.h"


namespace zschimmer {
namespace windows {

//------------------------------------------------------------------------------------------Service

struct Service : Object
{
  //typedef ptr<Service>        Constructor                 ( int argc, char** argv );


                                Service                     ()                                      : _zero_(this+1) {}
                               ~Service                     ()                                      {}

    int                         run_service                 ( const string& service_name, int argc, char** argv );

    void                    set_service_emulation_allowed   ( bool b )                              { _service_emulation_allowed = b; }
    void                    set_self_destruction_timeout    ( double t )                            { _self_destruction_timeout = t; }
    void                    set_service_status              ( int state, int checkpoint_value = 0, int wait_hint_seconds = 10 );
  //void                    set_start_duration_hint         ( double t )                            { _start_duration_hint = t; }
  //void                    set_stop_duration_hint          ( double t )                            { _stop_duration_hint = t; }
    string                      name                        () const                                { return _name; }

    virtual void                run                         ( int argc, char** argv )               = 0;
    virtual DWORD               service_control_handler     ( DWORD, DWORD, void* )                 = 0;
  //virtual void                service_start               ()                                      = 0;
  //virtual void                service_stop                ()                                      = 0;
  //virtual void                service_shutdown            ()                                      { service_stop(); }
  //virtual void                service_pause               ()                                      = 0;
  //virtual void                service_continue            ()                                      = 0;
    virtual void                abort_immediately           ()                                      {}

  private:
    static DWORD __stdcall      service_control_handler_static( DWORD, DWORD, void*, void* );       // HandlerEx
    DWORD                       service_control_handler_super ( DWORD, DWORD, void* );                    

    static void __stdcall       ServiceMain                 ( DWORD argc, char** argv );
    void                        service_main                ( DWORD argc, char** argv );

    static uint __stdcall       service_thread_static       ( void* );
    uint                        service_thread              ();

    void                        start_self_destruction      ();
    static uint __stdcall       self_destruction_thread_static( void* );
    uint                        self_destruction_thread     ();

    Fill_zero                  _zero_;
    string                     _name;
    bool                       _service_emulation_allowed;
    double                     _self_destruction_timeout;
    Thread_id                  _self_destruction_thread_id;
    bool                       _service_stop;               // STOP-Kommando von der Dienstesteuerung (und nicht von TCP-Schnittstelle)

    bool                       _abort_immediately;

    SERVICE_STATUS_HANDLE      _service_status_handle;
    Handle                     _thread_handle;
    string                     _service_name;
    int                        _argc;
    char**                     _argv;

    // Zustände SERVICE_START_PENDING und SERVICE_STOP_PENDING nicht länger als pending_timeout Sekunden:
    Event                      _service_main_terminated;
    windows::Handle            _pending_watchdog_signal;
    bool                       _pending_timed_out;
  //double                     _start_duration_hint;
  //double                     _stop_duration_hint;
    int                        _state;
};

//-------------------------------------------------------------------------------------------------

void                            install_service             ( const string& service_name, const string& service_display, const string& service_description, 
                                                              const string& dependencies, const string& params );
void                            remove_service              ( const string& service_name );
void                            start_service               ( const string& service_name );

string                          service_state_name          ( int state );

//-------------------------------------------------------------------------------------------------


} //namespace windows
} //namespace zschimmer

#endif
