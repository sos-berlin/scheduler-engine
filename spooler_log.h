// $Id: spooler_log.h,v 1.4 2002/03/02 19:22:55 jz Exp $

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

    void                        log                         ( Log_level, const string& prefix, const string& );
    void                        collect_stderr              ();
    
    string                      filename                    () const                            { return _filename; }

  protected:
    void                        write                       ( const char*, int len, bool log = true );
    void                        write                       ( const string& line )              { write( line.c_str(), line.length() ); }

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

    void                        set_prefix                  ( const string& prefix )            { _prefix = prefix; }

    void                        operator()                  ( const string& line )              { info( line ); }
    void                        debug                       ( const string& line )              { log( log_debug_spooler, line ); }
    void                        info                        ( const string& line )              { log( log_info , line ); }
    void                        warn                        ( const string& line )              { log( log_warn , line ); }
    void                        error                       ( const string& line )              { log( log_error, line ); }
    void                        log                         ( Log_level, const string& );

  protected:
    Log*                       _log;
  //Task*                      _task;
    string                     _prefix;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
