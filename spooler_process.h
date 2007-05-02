// $Id$

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Process_class;
struct Process_class_subsystem;

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

struct Process_class : idispatch_implementation< Process_class, spooler_com::Iprocess_class >,
                       Scheduler_object
{
                                Process_class               ( Process_class_subsystem*, const string& name = "" );
  //explicit                    Process_class               ( Spooler* sp, const xml::Element_ptr& e ) : _zero_(this+1), _spooler(sp) { init();  set_dom( e ); }
    Z_GNU_ONLY(                 Process_class               (); )
                               ~Process_class               ();

    void                        init                        ();
    void                        prepare_remove              ();

    void                        add_process                 ( Process* );
    void                        remove_process              ( Process* );


    Process*                    new_process                 ();
    Process*                    select_process_if_available ();                                     // Startet bei Bedarf. Bei _max_processes: return NULL
    bool                        process_available           ( Job* for_job );
    void                        enqueue_waiting_job         ( Job* );
    void                        remove_waiting_job          ( Job* );
    bool                        need_process                ();
    void                        notify_a_process_is_idle    ();
    void                    set_name                        ( const string& );
    string                      name                        () const                                { return _name; }
    string                      path                        () const                                { return _name; }
    void                    set_max_processes               ( int );
    void                    set_remote_scheduler            ( const Host_and_port& );
    const Host_and_port&        remote_scheduler            () const                                { return _remote_scheduler; }

    bool                        is_remote_host              () const                                { return _remote_scheduler; }
    bool                        is_added                    () const;
    void                        remove                      ();

    //void                        register_module             ( Remote_module_instance_proxy* module ){ _module_set.insert( module ); }
    //void                      unregister_module             ( Remote_module_instance_proxy* module ){ _module_set.erase( module ); }
    //bool                        is_any_module_registered    () const                                { return !_module_set.empty(); }

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        execute_xml                 ( const xml::Element_ptr&, const Show_what& );

    string                      obj_name                    () const;


    // spooler_com::Iprocess_class:
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Process_class"; }
    STDMETHODIMP            put_Name                        ( BSTR );
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( _name, result ); }
    STDMETHODIMP            put_Remote_scheduler            ( BSTR );
    STDMETHODIMP            get_Remote_scheduler            ( BSTR* result )                        { return String_to_bstr( _remote_scheduler.as_string(), result ); }
    STDMETHODIMP            put_Max_processes               ( int );
    STDMETHODIMP            get_Max_processes               ( int* result )                         { *result = _max_processes;  return S_OK; }
    STDMETHODIMP                Remove                      ();


    // Subsystem:
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

  private:
    Fill_zero                  _zero_;
    string                     _name;
    int                        _max_processes;
    
    //typedef stdext::hash_set<Remote_module_instance_proxy*> Module_set
    //Module_set                 _module_set;

    Job_list                   _waiting_jobs;
    Host_and_port              _remote_scheduler;

  public:
    Process_list               _process_list;


    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//--------------------------------------------------------------------------Process_class_subsystem

struct Process_class_subsystem : idispatch_implementation< Process_class_subsystem, spooler_com::Iprocess_classes>, 
                                 Subsystem
{
                                Process_class_subsystem     ( Scheduler* );

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    void                        add_process_class           ( Process_class* );
    void                        remove_process_class        ( Process_class* );
  //bool                        is_empty                    () const                                { return _lock_map.empty(); }
    Process_class*              process_class               ( const string& name );
    Process_class*              process_class_or_null       ( const string& name );
    Process*                    new_temporary_process       ();
    Process_class*              temporary_process_class     ()                                      { return _temporary_process_class; }
    bool                        has_process_classes         ()                                      { return _process_class_map.size() > 1; }   // Eine ist _temporary_process_class
    bool                        try_to_free_process         ( Job* for_job, Process_class*, const Time& now );
    bool                        async_continue              ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );
    void                        execute_xml_process_class   ( const xml::Element_ptr& );

    // spooler_com::Iprocess_classes
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Process_classes"; }
    STDMETHODIMP            get_Process_class               ( BSTR, spooler_com::Iprocess_class** );
    STDMETHODIMP            get_Process_class_or_null       ( BSTR, spooler_com::Iprocess_class** );
    STDMETHODIMP                Create_process_class        ( spooler_com::Iprocess_class** );
    STDMETHODIMP                Add_process_class           ( spooler_com::Iprocess_class* );


  private:
    Fill_zero                  _zero_;
    ptr<Process_class>         _temporary_process_class;

  public:
    typedef map< string, ptr<Process_class> > Process_class_map;
    Process_class_map                        _process_class_map;


    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
