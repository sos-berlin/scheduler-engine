// $Id: spooler_process.h,v 1.15 2003/10/28 22:51:26 jz Exp $

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace spooler {


struct Process_class;


//------------------------------------------------------------------------------------------Process

struct Process : zschimmer::Object
{
    //typedef object_server::Session Session;

                                Process                     ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}
    Z_GNU_ONLY(                 Process                     (); )


    bool                        started                     ()                                      { return _connection != NULL; }

    void                        start                       ();
    object_server::Session*     session                     ()                                      { return _session; }
  //void                    set_event                       ( Event* e )                            { if( _connection )  _connection->set_event( e ); }
    bool                        async_continue              ();
    double                      async_next_gmtime           ()                                      { return _connection->async_next_gmtime(); }
    void                        add_module_instance         ( Module_instance* );
    void                        remove_module_instance      ( Module_instance* );
    int                         module_instance_count       ()                                      { return _module_instance_count; }
    void                    set_temporary                   ( bool t )                              { _temporary = t; }
    void                    set_job_name                    ( const string& job_name )              { _job_name = job_name; }
    void                    set_task_id                     ( int id )                              { _task_id = id; }
    void                    set_server                      ( const string& hostname, int port )    { _server_hostname = hostname;  _server_port = port; }
    int                         pid                         ()                                      { return _connection? _connection->pid() : 0; }
    bool                        kill                        ();

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );

    
//private:
    Fill_zero                  _zero_;
    string                     _job_name;
    int                        _task_id;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;
    string                     _server_hostname;
    int                        _server_port;
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    Time                       _running_since;
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long                       _module_instance_count;
    Module_instance*           _module_instance;
    Process_class*             _process_class;
};

//-------------------------------------------------------------------------------------Process_list

typedef list< ptr<Process> >    Process_list;

//------------------------------------------------------------------------------------Process_class

struct Process_class : zschimmer::Object
{
                                Process_class               ( Spooler* sp, const string& name )        : _zero_(this+1), _spooler(sp), _name(name) {}
    explicit                    Process_class               ( Spooler* sp, const xml::Element_ptr& e ) : _zero_(this+1), _spooler(sp) { set_dom( e ); }
    Z_GNU_ONLY(                 Process_class               (); )

    
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
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    string                     _name;
    int                        _max_processes;
    Spooler*                   _spooler;
    Process_list               _process_list;
    Job_list                   _waiting_jobs;
};

//-------------------------------------------------------------------------------Process_class_list

typedef list< ptr<Process_class> >   Process_class_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
