// $Id: spooler_log.h,v 1.23 2002/11/25 23:36:21 jz Exp $

#ifndef __SPOOLER_LOG_H
#define __SPOOLER_LOG_H

namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------------Log_level
/*
enum Log_level
{ 
    log_debug = -1,     // alle Werte < 0 sind Debug-Ausgaben (debug1, debug2 etc.)
    log_info  =  0, 
    log_warn  =  1, 
    log_error =  2,
    log_fatal =  3
};
*/

Log_level                       make_log_level              ( const string& );

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

    void                        log                         ( Log_level, const string& prefix, const string& );
    void                        log2                        ( Log_level, const string& prefix, const string&, Prefix_log* = NULL );
    void                        collect_stderr              ();
    
    string                      filename                    () const                            { return _filename; }

  protected:
    void                        write                       ( Prefix_log*, const char*, int len, bool log = true );
    void                        write                       ( Prefix_log* extra, const string& line )              { write( extra, line.c_str(), line.length() ); }
    void                        write                       ( Prefix_log* extra, const char* line )                { write( extra, line, strlen(line) ); }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    string                     _filename;
    int                        _file;
  //FILE*                      _file;
    Thread_semaphore           _semaphore;
    string                     _log_line;
};

//---------------------------------------------------------------------------------------Prefix_log

struct Prefix_log
{
                                Prefix_log                  ( int );                            // Für Spooler
                                Prefix_log                  ( Spooler*, const string& prefix = empty_string );
                               ~Prefix_log                  ();

    void                        init                        ( Spooler*, const string& prefix = empty_string );
    void                        open                        ();
    void                        close                       ();
    void                        close2                      ();

    void                    set_append                      ( bool b )                          { _append = b; }
    void                    set_filename                    ( const string& );
    string                      filename                    () const                            { return _filename; }
    void                    set_new_filename                ( const string& );
    string                      new_filename                ()                                  { return _new_filename; }
    void                    set_log_level                   ( int level )                       { _log_level = level; }
    int                         log_level                   ()                                  { return _log_level; }
    void                        reset_highest_level         ()                                  { _highest_level = -999; }
    int                         highest_level               () const                            { return _highest_level; }
    string                      highest_msg                 () const                            { return _highest_msg; }

    void                    set_collect_within              ( Time time )                       { _collect_within = time; }
    Time                        collect_within              ()                                  { return _collect_within; }
    void                    set_collect_max                 ( Time time )                       { _collect_max = time; }
    Time                        collect_max                 ()                                  { return _collect_max; }
    Time                        collect_end                 ()                                  { return _first_send? _first_send + _collect_max : 0; }

    void                    set_job                         ( Job* job )                        { _job = job; }
    void                    set_prefix                      ( const string& prefix )            { _prefix = prefix; }
    void                    set_profile_section             ( const string& );

    void                        operator()                  ( const string& line )              { info( line ); }
    void                        debug9                      ( const string& line )              { log( log_debug9, line ); }
    void                        debug8                      ( const string& line )              { log( log_debug8, line ); }
    void                        debug7                      ( const string& line )              { log( log_debug7, line ); }
    void                        debug6                      ( const string& line )              { log( log_debug6, line ); }
    void                        debug5                      ( const string& line )              { log( log_debug5, line ); }
    void                        debug4                      ( const string& line )              { log( log_debug4, line ); }
    void                        debug3                      ( const string& line )              { log( log_debug3, line ); }
    void                        debug2                      ( const string& line )              { log( log_debug2, line ); }
    void                        debug1                      ( const string& line )              { log( log_debug1, line ); }
    void                        debug                       ( const string& line )              { log( log_debug_spooler, line ); }
    void                        info                        ( const string& line )              { log( log_info  , line ); }
    void                        warn                        ( const string& line )              { log( log_warn  , line ); }
    void                        error                       ( const string& line )              { log( log_error , line ); }
    void                        log                         ( Log_level, const string& );

    void                    set_mail_on_error               ( bool b )                          { _mail_on_error = b; }
    bool                        mail_on_error               ()                                  { return _mail_on_error; }

    void                    set_mail_on_success             ( bool b )                          { _mail_on_success = b; }
    bool                        mail_on_success             ()                                  { return _mail_on_success; }

    void                    set_mail_on_process             ( int level )                       { _mail_on_process = level; }
    int                         mail_on_process             ()                                  { return _mail_on_process; }

#ifdef Z_WINDOWS
    Com_mail*                   imail                       ();
#endif

    // Defaults setzen, ohne eMail-Objekt anzulegen:
    void                    set_mail_from_name              ( const string& );
    void                    set_mail_subject                ( const string&, bool overwrite = false );
    void                    set_mail_body                   ( const string&, bool overwrite = false );

    void                        send                        ( int reason );
    void                        send_really                 ();

    friend struct               Log;

  protected:

    void                        write                       ( const char*, int );
    void                        set_mail_header             ();


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Log*                       _log;
    Job*                       _job;
    string                     _prefix;
    string                     _section;
    int                        _log_level;                  // Ab diesem Level protokollieren, sonst nicht
    int                        _highest_level;
    string                     _highest_msg;

    string                     _filename;                   // Name einer zusätzlichen Log-Datei (für die Tasks)
    string                     _new_filename;               // nach close() umbenennen
    bool                       _append;                     // Datei zum Fortschreiben öffnen
    int                        _file;                       // File handle

    bool                       _mail_on_error;
    bool                       _mail_on_success;
    int                        _mail_on_process;
#ifdef Z_WINDOWS
    ptr<Com_mail>              _mail;
#endif
    string                     _mail_section;               // Name des Abschnitts in factory.ini für eMail-Einstellungen

    Time                       _collect_within;             // eMails innerhalb dieser Frist sammeln, solange Job keinen Fehler macht
    Time                       _last_send;                  // Beginn dieser Frist
    Time                       _collect_max;                // Nach dieser Frist eMail auf jeden Fall versenden
    Time                       _first_send;                 // Beginn dieser Frist

    string                     _smtp_server;                // Aus factory.ini [Job ...]
    string                     _queue_dir;
    string                     _to, _cc, _bcc, _from;
    string                     _from_name;
    string                     _subject;
    string                     _body;

    string                     _log_buffer;                 // Für Jobprotokollausgaben bis open(), also vor dem Jobstart
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
