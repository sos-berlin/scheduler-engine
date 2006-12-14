// $Id$

#ifndef __SPOOLER_EVENT_H
#define __SPOOLER_EVENT_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

enum Scheduler_event_type
{
    evt_none,
    evt_unknown,
    evt_scheduler_state_changed,
    evt_scheduler_started,
    evt_scheduler_fatal_error,
    evt_scheduler_kills,
    evt_job_error,
    evt_task_ended,
    evt_disk_full,
    evt_database_error,
    evt_database_error_switch_to_file,
    evt_database_error_abort,
    evt_database_continue,
    evt_task_start_error,
    evt_file_order_source_error,
    evt_file_order_source_recovered,
    evt_file_order_error,
    evt_order_state_changed,
};

//----------------------------------------------------------------------------------Scheduler_event

struct Scheduler_event
{
    static string               name_of_event_code          ( Scheduler_event_type );


                                Scheduler_event             ( Scheduler_event_type, Log_level severity, Scheduler_object* );

    Time                        timestamp                   ();

    void                    set_mail                        ( Com_mail* );
    Com_mail*                   mail                        ();
    void                    set_scheduler_terminates        ( bool b )                              { _scheduler_terminates = b; }
    void                    set_error                       ( const Xc_copy& x )                    { _error    = x; }
    void                    set_message                     ( const string& m )                     { _message  = m; }
    void                    set_count                       ( int c )                               { _count    = c; }
    void                    set_log_path                    ( const string& path )                  { _log_path = path; }
  //void                    set_subject                     ( const string& subject )               { _mail_defaults[ "subject" ] = remove_password( subject ); }
  //void                    set_body                        ( const string& body )                  { _mail_defaults[ "body"    ] = remove_password( body    ); }

    xml::Document_ptr           dom                         ();
    xml::Document_ptr           mail_dom                    ( const xml::Document_ptr& event = xml::Document_ptr() );  // Default: dom()
    void                        make_xml                    ();
    void                        print_xml                   ( String_stream* );
    const string&               xml                         () const                                { return _xml; }
  //int                         send_mail                   ( const xml::Document_ptr& mail  = xml::Document_ptr() );  // Default: mail_dom()
    int                         send_mail                   ( const Mail_defaults& );

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Scheduler_event_type                 _event_code;
    Time                       _timestamp;
    Log_level                  _severity;
    bool                       _scheduler_terminates;
    Scheduler_object*          _object;
    ptr<IUnknown>              _object_iunknown;            // Hält das Objekt (IUnknown ist die gemeinsame eindeutige Oberklasse)
    string                     _log_path;
    Xc_copy                    _error;
    string                     _message;
    int                        _count;
    ptr<Com_mail>              _mail;                       
    string                     _xml;
};

//-------------------------------------------------------------------------Scheduler_event_manager 

struct Scheduler_event_manager : Object, 
                                 Scheduler_object
{
                                Scheduler_event_manager     ( Spooler* );

    // Scheduler_object:
    Prefix_log*                 log                         ()                                      { return _log; }

    void                        open                        ();
    void                        close_responses             ();

    void                        report_event                ( Scheduler_event* );
    bool                        events_requested            ()                                      { return _get_events_command_response_list.size() > 0; }
    void                        add_get_events_command_response( Get_events_command_response* );
    void                        remove_get_events_command_response( Get_events_command_response* );
    
  private:
    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    z::File                    _event_file;

    typedef std::list< Get_events_command_response* >    Get_events_command_response_list;
    Get_events_command_response_list                    _get_events_command_response_list;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif


