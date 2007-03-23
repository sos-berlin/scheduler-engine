// $Id$

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace scheduler {


struct Process_class;


//------------------------------------------------------------------------------------------Process
// Ein Prozess, in dem ein Module oder eine Task ablaufen kann.

struct Process : zschimmer::Object, Scheduler_object
{
    struct Close_operation : Async_operation
    {
        enum State { s_initial, s_closing_session, s_closing_remote_process, s_finished };


                                    Close_operation         ( Process*, bool run_independently );
                                   ~Close_operation         ();

        // Async_operation:
        bool                        async_continue_         ( Continue_flags );
        bool                        async_finished_         () const;
        string                      async_state_text_       () const;

        static string               string_from_state       ( State );

      private:
        friend struct               Process;

        Fill_zero                  _zero_;
        State                      _state;
        ptr<Process>               _process;
        Async_operation*           _close_session_operation;
        ptr<Close_operation>       _hold_self;              // Objekt hält sich selbst, wenn es selbstständig, ohne Antwort, den Process schließen soll
    };


    struct Async_remote_operation : Async_operation
    {
        enum State
        {
            s_not_connected,
            s_connecting,
            s_starting,
            s_running,
            s_closing,
            s_closed
        };

        static string           state_name                  ( State );


                                Async_remote_operation      ( Process* );
                               ~Async_remote_operation      ();

        virtual bool            async_continue_             ( Continue_flags f )                    { return _process->async_remote_start_continue( f ); }
        virtual bool            async_finished_             () const                                { return _state == s_running  ||  _state == s_closed; }
        virtual string          async_state_text_           () const;

        void                    close_remote_task           ( bool kill = false );


        Fill_zero              _zero_;
        State                  _state;
        Process*               _process;
    };



                                Process                     ( Spooler* );
    Z_GNU_ONLY(                 Process                     (); )
                               ~Process                     ();


    void                        close_async                 ();
    Async_operation*            close__start                ( bool run_independently = false );
    void                        close__end                  ();
    bool                     is_closing                     ()                                      { return _close_operation != NULL; }
    bool                        continue_close_operation    ( Process::Close_operation* );


    bool                        started                     ()                                      { return _connection != NULL; }

    void                    set_controller_address          ( const Host_and_port& h )              { _controller_address = h; }
  //void                    set_stdin_data                  ( const string& data )                  { _stdin_data = data; }
    void                        start                       ();
    void                        start_local                 ();
    void                        async_remote_start          ();
    bool                        is_started                  ();
    bool                        async_remote_start_continue ( Async_operation::Continue_flags );
    object_server::Session*     session                     ()                                      { return _session; }
  //void                    set_event                       ( Event* e )                            { if( _connection )  _connection->set_event( e ); }
    bool                        async_continue              ();
    double                      async_next_gmtime           ()                                      { return _connection? _connection->async_next_gmtime() : (double)Time::never; }
    void                        add_module_instance         ( Module_instance* );
    void                        remove_module_instance      ( Module_instance* );
    int                         module_instance_count       ()                                      { return _module_instance_count; }
    void                    set_temporary                   ( bool t )                              { _temporary = t; }
    void                    set_job_name                    ( const string& job_name )              { _job_name = job_name; }
    void                    set_task_id                     ( int id )                              { _task_id = id; }
    void                    set_server                      ( const string& hostname, int port )    { _server_hostname = hostname;  _server_port = port; }
    void                    set_priority                    ( const string& priority )              { _priority = priority; }
    int                         pid                         () const                                { return _connection? _connection->pid() : 0; }
    bool                     is_terminated                  ();
    bool                        kill                        ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    File_path                   stderr_path                 ();
    File_path                   stdout_path                 ();
    bool                        connected                   ()                                      { return _connection? _connection->connected() : false; }
    bool                        is_remote_host              () const;

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    string                      obj_name                    () const;
    string                      short_name                  () const;

    
//private:
    Fill_zero                  _zero_;
    string                     _job_name;
    int                        _task_id;
    string                     _server_hostname;
    int                        _server_port;
    Host_and_port              _controller_address;
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    Process_handle             _process_handle_copy;
    bool                       _is_killed;
    int                        _exit_code;
    int                        _termination_signal;
    Time                       _running_since;
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long32                     _module_instance_count;
    Module_instance*           _module_instance;
    Process_class*             _process_class;
    string                     _priority;
    pid_t                      _remote_pid;
    File                       _remote_stdout_file;
    File                       _remote_stderr_file;
    ptr<Async_remote_operation> _async_remote_operation;
    ptr<Xml_client_connection>  _xml_client_connection;
    ptr<Close_operation>       _close_operation;
};

//-------------------------------------------------------------------------------------Process_list

typedef list< ptr<Process> >    Process_list;

//------------------------------------------------------------------------------------Process_class
// <process_class>

struct Process_class : zschimmer::Object
{
                                Process_class               ( Spooler* sp, const string& name )        : _zero_(this+1), _spooler(sp), _name(name) { init(); }
    explicit                    Process_class               ( Spooler* sp, const xml::Element_ptr& e ) : _zero_(this+1), _spooler(sp) { init();  set_dom( e ); }
    Z_GNU_ONLY(                 Process_class               (); )

    void                        init                        ();

    void                        add_process                 ( Process* );
    void                        remove_process              ( Process* );

    Process*                    new_process                 ();
    Process*                    select_process_if_available ();                                     // Startet bei Bedarf. Bei _max_processes: return NULL
    bool                        process_available           ( Job* for_job );
    void                        enqueue_waiting_job         ( Job* );
    void                        remove_waiting_job          ( Job* );
    bool                        need_process                ();
    void                        notify_a_process_is_idle    ();
  //Job*                        first_waiting_job           ()                                      { return _waiting_jobs.begin(); }
    string                      name                        ()                                      { return _name; }
    bool                        is_remote_host              () const                                { return _remote_scheduler; }

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    Fill_zero                  _zero_;
    string                     _name;
    int                        _max_processes;
    Spooler*                   _spooler;
    Process_list               _process_list;
    int                        _module_use_count;
    Job_list                   _waiting_jobs;
    Host_and_port              _remote_scheduler;
};

//-------------------------------------------------------------------------------Process_class_list

typedef list< ptr<Process_class> >   Process_class_list;

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
