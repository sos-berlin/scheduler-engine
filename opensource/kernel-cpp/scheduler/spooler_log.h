// $Id: spooler_log.h 15045 2011-08-26 07:09:06Z jz $

#ifndef __SPOOLER_LOG_H
#define __SPOOLER_LOG_H

#include "log_cache_Request.h"
#include "log_cache_Request_cache.h"

namespace sos {
namespace scheduler {

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
    void                        log2                        ( Log_level, bool log_to_files, const string& prefix, const string& line, 
                                                              Prefix_log* extra_log = NULL, Prefix_log* order_log = NULL );
    void                        collect_stderr              ();
    
    File_path                   filename                    () const                            { return _filename; }
    int                         fd                          ()                                  { return _file; }
    void                        start_new_file              ();
    Time                        last_time                   () const                            { return _last_time; }

  protected:
    friend struct               Prefix_log;                 // _semaphore

    void                        write                       ( Log_level  , Prefix_log* extra, Prefix_log* order, const char*, int len );
    void                        write                       ( Log_level l, Prefix_log* extra, Prefix_log* order, const string& line )       { write( l, extra, order, line.c_str(), line.length() ); }
    void                        write                       ( Log_level l, Prefix_log* extra, Prefix_log* order, const char* line )         { write( l, extra, order, line, strlen(line) ); }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    File_path                  _filename;
    int                        _file;
    int                        _err_no;
    Thread_semaphore           _semaphore;
  //string                     _log_line;
    string                     _log_buffer;                 // Bis Ausgabedatei geöffnet ist
    Time                       _last_time;
};

//---------------------------------------------------------------------------------------Prefix_log

struct Prefix_log : Object, Has_log, javabridge::has_proxy<Prefix_log>
{
    Fill_zero _zero_;


                                Prefix_log                  ( int );                            // Für Spooler
                                Prefix_log                  ( Scheduler_object* );
                               ~Prefix_log                  ();

    void                        init                        ( Scheduler_object*, const string& prefix = empty_string );
    void                        open                        ();
    void                        open_dont_cache             ();
    void                        close                       ();
    void                        finish_log                  ();
    void                        close_file                  ();     // Nicht öffentlich
    void                        try_reopen_file             ();     // Nicht öffentlich
    void                        remove_file                 ();
    bool                        started                     () const                            { return _started; }        // open() gerufen
    bool                        is_active                   () const                            { return _started && !_is_finished; }
    bool                        file_is_opened              () const                            { return _file != -1; }
    bool                        is_finished                 () const                            { return _is_finished; }
    bool                        is_stderr                   () const                            { return _file == fileno(stderr); }

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                    set_append                      ( bool b )                          { _append = b; }
    void                    set_append_for_cache            ( bool b )                          { _append_for_cache = b; }
    void                    set_filename                    ( const File_path& );
    File_path                   filename                    () const                            { return _filename == "" && _log? _log->filename() : _filename; }
    const File_path&            this_filename               () const                            { return _filename; }
    void                    set_title                       ( const string& title )             { _title = title; }
    string                      title                       ()                                  { return _title; }
    void                    set_remove_after_close          ( bool b )                          { _remove_after_close = b; }
    void                    set_new_filename                ( const string& );
    string                      new_filename                ()                                  { return _new_filename; }
    void                        start_new_file              ();
    int                         log_level                   ();
    bool                        is_enabled_log_level        ( Log_level );
    void                        reset_highest_level         ()                                  { _highest_level = (Log_level)-999; }
    Log_level                   highest_level               () const                            { return _highest_level; }
    string                      highest_msg                 () const                            { return _highest_msg; }
    int                         instance_number             () const                            { return _instance_number; }

    void                    set_collect_within              ( const Duration& d )               { _collect_within = d; }
    Duration                    collect_within              ()                                  { return _collect_within; }
    void                    set_collect_max                 ( const Duration& d )               { _collect_max = d; }
    Duration                    collect_max                 ()                                  { return _collect_max; }
    Time                        collect_end                 ()                                  { return _first_send.not_zero()? _first_send + _collect_max : Time(0); }

    void                        inherit_settings            ( const Prefix_log& );
    void                    set_job_name                    ( const string& job_name )          { _job_name = job_name; }
    void                    set_task                        ( Task* task )                      { _task = task; }
    void                    set_prefix                      ( const string& prefix )            { _prefix = prefix; }
    void                    set_profile_section             ( const string& );
    void                    set_dom_settings                ( xml::Element_ptr settings_element );
    void                    set_order_log                   ( Prefix_log* log )                 { _order_log = log; }

    void                        add_event                   ( Event_base* );
    void                        remove_event                ( Event_base* );
    void                        signal_events               ();

    void                        debug                       ( const string& line )              { log( log_debug_spooler, line ); }
    void                        log2                        ( Log_level, const string& prefix, const string& line, Has_log* );

    string                      last_error_line             ()                                  { return last( log_error ); }
    string                      last                        ( Log_level level ) const           { Last::const_iterator it = _last.find( level );
                                                                                                  return it == _last.end()? "" : it->second; }
    string                      last_line                   () const                            { return last( _last_level ); }
    bool                        has_line_for_level          ( Log_level level ) const           { return _last.find( level ) != _last.end(); }
    string                      java_last                   (const string& level) const         { return last(make_log_level(level)); }

    void                        continue_with_text          ( const string& );
    
    string                      as_string                   ();
    string                      as_string_ignore_error      ();

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

    void                    set_mail_defaults               ();
    void                    set_mail_default                ( const string& field_name, const string& value, bool overwrite = true );

    void                        send                        ( Scheduler_event* );
    void                        send_really                 ( Scheduler_event* );
    PrefixLogJ&                 typed_java_sister           ();
    string                      obj_name                    () const;



    bool                       _in_log;                     // log2() ist aktiv, Rekursion vermeiden!

  protected:
    void                        open_patiently_file         ();
    void                        open_file_without_error     ();
    void                        open_file                   ();
    void                        check_open_errno            ();
    void                        write                       ( const char*, int );

    friend struct               Log;
    friend struct               Task;                       // Für _mail_on_error etc.


    PrefixLogJ                 _typed_java_sister;
    Scheduler_object*          _object;
    Spooler*                   _spooler;
    string                     _job_name;
    Task*                      _task;
    Log*                       _log;
    Prefix_log*                _order_log;
    string                     _prefix;
    string                     _section;
    Log_level                  _highest_level;
    Log_level                  _last_level;
    string                     _highest_msg;
    
    typedef stdext::hash_map< Log_level, string >   Last;
    Last                       _last;

    string                     _title;
    File_path                  _filename;                   // Name einer zusätzlichen Log-Datei (für die Tasks)
    File_path                  _new_filename;               // nach close() umbenennen
    bool                       _append;                     // Datei zum Fortschreiben ?ffnen
    bool                       _append_for_cache;           // Datei zum Fortschreiben ?ffnen
    int                        _file;                       // File handle
    ptr<log::cache::Request>   _inhibit_caching_request;
    int                        _instance_number;
    int                        _err_no;
    bool                       _started;                    // open() gerufen
    bool                       _closed;
    bool                       _is_finished;
    bool                       _open_and_close_every_line;
    bool                       _mail_defaults_set;
    bool                       _mail_on_warning;
    bool                       _mail_on_error;
    bool                       _mail_on_success;
    int                        _mail_on_process;
    First_and_last             _mail_on_delay_after_error;
    bool                       _mail_it;
    ptr<Com_mail>              _mail;
    string                     _mail_section;               // Name des Abschnitts in factory.ini für eMail-Einstellungen

    Duration                   _collect_within;             // eMails innerhalb dieser Frist sammeln, solange Job keinen Fehler macht
    Time                       _last_send;                  // Beginn dieser Frist
    Duration                   _collect_max;                // Nach dieser Frist eMail auf jeden Fall versenden
    Time                       _first_send;                 // Beginn dieser Frist

    Mail_defaults              _mail_defaults;

    bool                       _is_logging_continuing;
    string                     _log_buffer;                 // Für Jobprotokollausgaben bis open(), also vor dem Jobstart
    bool                       _log_buffer_is_full;

    bool                       _remove_after_close;
    list<Event_base*>          _events;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
