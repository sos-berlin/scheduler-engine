// $Id$

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace spooler {


struct Process_class;


//------------------------------------------------------------------------------------------Process
// Ein Prozess, in dem ein Module oder eine Task ablaufen kann.

struct Process : zschimmer::Object, Scheduler_object
{
    //typedef object_server::Session Session;

                                Process                     ( Spooler* );
    Z_GNU_ONLY(                 Process                     (); )
                               ~Process                     ();


    bool                        started                     ()                                      { return _connection != NULL; }

    void                        start                       ();
    object_server::Session*     session                     ()                                      { return _session; }
  //void                    set_event                       ( Event* e )                            { if( _connection )  _connection->set_event( e ); }
    bool                        async_continue              ();
    double                      async_next_gmtime           ()                                      { return _connection? _connection->async_next_gmtime() : (double)latter_day; }
    void                        add_module_instance         ( Module_instance* );
    void                        remove_module_instance      ( Module_instance* );
    int                         module_instance_count       ()                                      { return _module_instance_count; }
    void                    set_temporary                   ( bool t )                              { _temporary = t; }
    void                    set_job_name                    ( const string& job_name )              { _job_name = job_name; }
    void                    set_task_id                     ( int id )                              { _task_id = id; }
    void                    set_server                      ( const string& hostname, int port )    { _server_hostname = hostname;  _server_port = port; }
    void                    set_priority                    ( const string& priority )              { _priority = priority; }
    int                         pid                         () const                                { return _connection? _connection->pid() : 0; }
    bool                        kill                        ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    string                      stderr_path                 ();
    string                      stdout_path                 ();

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const                                { return "Process " + as_string( pid() ); }

    
//private:
    Fill_zero                  _zero_;
    string                     _job_name;
    int                        _task_id;
    Thread_semaphore           _lock;
    ptr<Prefix_log>            _log;
    string                     _server_hostname;
    int                        _server_port;
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    Process_handle             _process_handle_copy;
    int                        _exit_code;
    int                        _termination_signal;
    Time                       _running_since;
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long32                     _module_instance_count;
    Module_instance*           _module_instance;
    Process_class*             _process_class;
    string                     _priority;
};

//-------------------------------------------------------------------------------------Process_list

typedef list< ptr<Process> >    Process_list;

//------------------------------------------------------------------------------------Process_class
// <process_class>

struct Process_class : zschimmer::Object
{
                                Process_class               ( Spooler* sp, const string& name )        : _zero_(this+1), _spooler(sp), _name(name), _lock("Process_class " + name) { init(); }
    explicit                    Process_class               ( Spooler* sp, const xml::Element_ptr& e ) : _zero_(this+1), _spooler(sp), _lock("Process_class") { init();  set_dom( e ); }
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

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    string                     _name;
    int                        _max_processes;
    Spooler*                   _spooler;
    Process_list               _process_list;
    int                        _module_use_count;
    Job_list                   _waiting_jobs;
};

//-------------------------------------------------------------------------------Process_class_list

typedef list< ptr<Process_class> >   Process_class_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
