// $Id: spooler_log.h,v 1.5 2002/03/02 20:15:02 jz Exp $

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

    void                        log                         ( Log_level, const string& prefix, const string&, Prefix_log* = NULL );
    void                        collect_stderr              ();
    
    string                      filename                    () const                            { return _filename; }

  protected:
    void                        write                       ( Prefix_log*, const char*, int len, bool log = true );
    void                        write                       ( Prefix_log* extra, const string& line )              { write( extra, line.c_str(), line.length() ); }

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    string                     _filename;
    int                        _file;
  //FILE*                      _file;
    Thread_semaphore           _semaphore;
};

//---------------------------------------------------------------------------------------Prefix_log

struct Prefix_log
{
                                Prefix_log                  ( Log*, const string& prefix = empty_string );
                               ~Prefix_log                  ();

    void                        close                       ();

    void                        open                        ( const string& filename );
    const string&               filename                    () const                            { return _filename; }
    void                        set_prefix                  ( const string& prefix )            { _prefix = prefix; }

    void                        operator()                  ( const string& line )              { info( line ); }
    void                        debug                       ( const string& line )              { log( log_debug_spooler, line ); }
    void                        info                        ( const string& line )              { log( log_info , line ); }
    void                        warn                        ( const string& line )              { log( log_warn , line ); }
    void                        error                       ( const string& line )              { log( log_error, line ); }
    void                        log                         ( Log_level, const string& );

    friend struct               Log;

  protected:
    Fill_zero                  _zero_;
    Log*                       _log;
    string                     _prefix;

    string                     _filename;                   // Name einer zusätzlichen Log-Datei (für die Tasks)
    bool                       _append;                     // Datei zum Fortschreiben öffnen
    int                        _file;                       // File handle
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
