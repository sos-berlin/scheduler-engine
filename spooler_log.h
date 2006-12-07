// $Id$

#ifndef __SPOOLER_LOG_H
#define __SPOOLER_LOG_H

namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------------------Log

struct Log 
{
                                Log                         ( Spooler* );
                               ~Log                         ();

    void                        set_directory               ( const string& );
    void                        open_new                    ();

    void                        debug                       ( const string& line )              { log( log_debug_spooler, "", line ); }   // Spooler-intern: debug3, aber COM: debug1
    void                        info                        ( const string& line )              { log( log_info  , "", line ); }
    void                        warn                        ( const string& line )              { log( log_warn  , "", line ); }
    void                        error                       ( const string& line )              { log( log_error , "", line ); }

    void                        log                         ( Log_level, const string& prefix, const string& line );
    void                        log2                        ( Log_level, const string& prefix, const string& line, 
                                                              Prefix_log* extra_log = NULL, Prefix_log* order_log = NULL );
    void                        collect_stderr              ();
    
    string                      filename                    () const                            { return _filename; }
    int                         fd                          ()                                  { return _file; }
    void                        start_new_file              ();
    Gmtime                      last_time                   () const                            { return _last_time; }

  protected:
    friend struct               Prefix_log;                 // _semaphore

    void                        write                       ( Prefix_log* extra, Prefix_log* order, const char*, int len, bool log = true );
    void                        write                       ( Prefix_log* extra, Prefix_log* order, const string& line )              { write( extra, order, line.c_str(), line.length() ); }
    void                        write                       ( Prefix_log* extra, Prefix_log* order, const char* line )                { write( extra, order, line, strlen(line) ); }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    string                     _filename;
    int                        _file;
    int                        _err_no;
    Thread_semaphore           _semaphore;
  //string                     _log_line;
    string                     _log_buffer;                 // Bis Ausgabedatei geöffnet ist
    Gmtime                     _last_time;
};

//---------------------------------------------------------------------------------------Prefix_log

struct Prefix_log : Object, Has_log
{
    Fill_zero _zero_;

                                Prefix_log                  ( int );                            // Für Spooler
                                Prefix_log                  ( Scheduler_object*, const string& prefix = empty_string );
                               ~Prefix_log                  ();

    void                        init                        ( Scheduler_object*, const string& prefix = empty_string );
    void                        open                        ();
    void                        close                       ();
    void                        close_file                  ();
    bool                        started                     () const                            { return _started; }        // open() gerufen
    bool                        opened                      () const                            { return _file != -1; }
    bool                        closed                      () const                            { return _closed; }
    bool                        is_stderr                   () const                            { return _file == fileno(stderr); }

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                    set_append                      ( bool b )                          { _append = b; }
    void                    set_filename                    ( const string& );
    string                      filename                    () const                            { return _filename == "" && _log? _log->filename() : _filename; }
    void                    set_title                       ( const string& title )             { _title = title; }
    string                      title                       ()                                  { return _title; }
    void                    set_remove_after_close          ( bool b )                          { _remove_after_close = b; }
    void                    set_new_filename                ( const string& );
    string                      new_filename                ()                                  { return _new_filename; }
    void                        start_new_file              ();
  //void                    set_log_level                   ( Log_level level )                 { _log_level = level; }
    int                         log_level                   ();
    void                        reset_highest_level         ()                                  { _highest_level = (Log_level)-999; }
    Log_level                   highest_level               () const                            { return _highest_level; }
    string                      highest_msg                 () const                            { return _highest_msg; }

    void                    set_collect_within              ( Time time )                       { _collect_within = time; }
    Time                        collect_within              ()                                  { return _collect_within; }
    void                    set_collect_max                 ( Time time )                       { _collect_max = time; }
    Time                        collect_max                 ()                                  { return _collect_max; }
    Time                        collect_end                 ()                                  { return _first_send? _first_send + _collect_max : Time(0); }

    void                        inherit_settings            ( const Prefix_log& );
    void                    set_job_name                    ( const string& job_name )          { _job_name = job_name; }
    void                    set_task                        ( Task* task )                      { _task = task; }
    void                    set_prefix                      ( const string& prefix )            { _prefix = prefix; }
    void                    set_profile_section             ( const string& );
    void                    set_order_log                   ( Prefix_log* log )                 { _order_log = log; }

    void                        add_event                   ( Event_base* );
    void                        remove_event                ( Event_base* );
    void                        signal_events               ();

  //void                        operator()                  ( const string& line )              { info( line ); }
  //void                        debug9                      ( const string& line )              { log( log_debug9, line ); }
  //void                        debug8                      ( const string& line )              { log( log_debug8, line ); }
  //void                        debug7                      ( const string& line )              { log( log_debug7, line ); }
  //void                        debug6                      ( const string& line )              { log( log_debug6, line ); }
  //void                        debug5                      ( const string& line )              { log( log_debug5, line ); }
  //void                        debug4                      ( const string& line )              { log( log_debug4, line ); }
  //void                        debug3                      ( const string& line )              { log( log_debug3, line ); }
  //void                        debug2                      ( const string& line )              { log( log_debug2, line ); }
  //void                        debug1                      ( const string& line )              { log( log_debug1, line ); }
    void                        debug                       ( const string& line )              { log( log_debug_spooler, line ); }
  //void                        info                        ( const string& line )              { log( log_info  , line ); }
  //void                        warn                        ( const string& line )              { log( log_warn  , line ); }
  //void                        error                       ( const string& line )              { log( log_error , line ); }
  //void                        log                         ( Log_level level, const string& line )  { Has_log::log( level, line ); }
    void                        log2                        ( Log_level, const string& prefix, const string& line, Has_log* );

    string                      last_error_line             ()                                  { return last( log_error ); }
    string                      last                        ( Log_level level ) const           { Last::const_iterator it = _last.find( level );
                                                                                                  return it == _last.end()? "" : it->second; }
    string                      last_line                   () const                            { return last( _last_level ); }

    string                      as_string                   ();

    void                    set_mail_on_error               ( bool b )                          { _mail_on_error = b; }
    bool                        mail_on_error               ()                                  { return _mail_on_error; }

    void                    set_mail_on_warning             ( bool b )                          { _mail_on_warning = b; }
    bool                        mail_on_warning             ()                                  { return _mail_on_warning; }

    void                    set_mail_on_success             ( bool b )                          { _mail_on_success = b; }
    bool                        mail_on_success             ()                                  { return _mail_on_success; }

    void                    set_mail_on_process             ( int level )                       { _mail_on_process = level; }
    int                         mail_on_process             ()                                  { return _mail_on_process; }

    void                    set_mail_it                     ( bool b )                          { _mail_it = b; }

    Com_mail*                   imail                       ();

    // Defaults setzen, ohne eMail-Objekt anzulegen:
  //void                    set_mail_from_name              ( const string&, bool overwrite = false );
  //void                    set_mail_subject                ( const string&, bool overwrite = false );
  //void                    set_mail_body                   ( const string&, bool overwrite = false );
  //string                      mail_to                     () const                            { return _to; }
  //string                      mail_from                   () const                            { return _from; }

    void                    set_mail_defaults               ();
    void                    set_mail_default                ( const string& field_name, const string& value, bool overwrite = true );

    void                        send                        ( int reason, Scheduler_event* );
    void                        send_really                 ( Scheduler_event* );



    bool                       _in_log;                     // log2() ist aktiv, Rekursion vermeiden!

  protected:

    void                        write                       ( const char*, int );
  //void                        set_mail_header             ();

    friend struct               Log;


    Scheduler_object*          _object;
    Spooler*                   _spooler;
    string                     _job_name;
    Task*                      _task;
    Log*                       _log;
    Prefix_log*                _order_log;
    string                     _prefix;
    string                     _section;
  //Log_level                  _log_level;                  // Ab diesem Level protokollieren, sonst nicht
    Log_level                  _highest_level;
    Log_level                  _last_level;
    string                     _highest_msg;
    
    typedef stdext::hash_map< Log_level, string >   Last;
    Last                       _last;

    string                     _title;
    string                     _filename;                   // Name einer zusätzlichen Log-Datei (für die Tasks)
    string                     _new_filename;               // nach close() umbenennen
    bool                       _append;                     // Datei zum Fortschreiben öffnen
    int                        _file;                       // File handle
    int                        _err_no;
    bool                       _started;                    // open() gerufen
    bool                       _closed;
    bool                       _mail_defaults_set;
    bool                       _mail_on_warning;
    bool                       _mail_on_error;
    bool                       _mail_on_success;
    int                        _mail_on_process;
    bool                       _mail_it;
    ptr<Com_mail>              _mail;
    string                     _mail_section;               // Name des Abschnitts in factory.ini für eMail-Einstellungen

    Time                       _collect_within;             // eMails innerhalb dieser Frist sammeln, solange Job keinen Fehler macht
    Time                       _last_send;                  // Beginn dieser Frist
    Time                       _collect_max;                // Nach dieser Frist eMail auf jeden Fall versenden
    Time                       _first_send;                 // Beginn dieser Frist

    Mail_defaults              _mail_defaults;

    string                     _log_buffer;                 // Für Jobprotokollausgaben bis open(), also vor dem Jobstart

    bool                       _remove_after_close;
    list<Event_base*>          _events;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
