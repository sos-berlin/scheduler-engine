// $Id: spooler_process.h 3383 2005-03-05 23:27:38Z jz $

#ifndef __SPOOLER_SUBPROCESS_H
#define __SPOOLER_SUBPROCESS_H


namespace sos {
namespace spooler {


struct Process_class;


//---------------------------------------------------------------------------------------Subprocess
// Ein vom einem Job gestarteter Prozess (mit irgendeinem fremden Programm).

struct Subprocess : zschimmer::Process
{
                                Subprocess                  ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}
    Z_GNU_ONLY(                 Subprocess                  (); )
                               ~Subprocess                  ();


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
    int                         pid                         ()                                      { return _connection? _connection->pid() : 0; }
    bool                        kill                        ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    string                      stderr_filename             ();
    string                      stdout_filename             ();

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, const Show_what& );

    
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
    Process_handle             _process_handle_copy;
    int                        _exit_code;
    int                        _termination_signal;
    Time                       _running_since;
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long                       _module_instance_count;
    Module_instance*           _module_instance;
    Process_class*             _process_class;
};

//-------------------------------------------------------------------------------------Process_list

typedef list< ptr<Subprocess> >    Process_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
