// $Id: spooler_log.h,v 1.1 2001/01/25 20:28:38 jz Exp $

#ifndef __SPOOLER_LOG_H
#define __SPOOLER_LOG_H

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------------Log_kind

enum Log_kind 
{ 
    log_msg, 
    log_warn, 
    log_error 
};

//----------------------------------------------------------------------------------------------Log

struct Log
{
                                Log                         ( Spooler* );
                               ~Log                         ();

    void                        set_directory               ( const string& );
    void                        open_new                    ();

    void                        msg                         ( const string& line )              { log( log_msg, "", line ); }
    void                        warn                        ( const string& line )              { log( log_warn, "", line ); }
    void                        error                       ( const string& line )              { log( log_error, "", line ); }

    void                        log                         ( Log_kind log, const string& prefix, const string& );
    
    string                      filename                    () const                            { return _filename; }

  protected:
    void                        write                       ( const string& );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    string                     _directory;
    string                     _filename;
    FILE*                      _file;
    Thread_semaphore           _semaphore;
};

//---------------------------------------------------------------------------------------Prefix_log

struct Prefix_log
{
                                Prefix_log                  ( Log*, const string& prefix = empty_string );

    void                        set_prefix                  ( const string& prefix )            { _prefix = prefix; }
    void                        msg                         ( const string& line )              { log( log_msg, line ); }
    void                        warn                        ( const string& line )              { log( log_warn, line ); }
    void                        error                       ( const string& line )              { log( log_error, line ); }
    void                        log                         ( Log_kind log, const string& );

  protected:
    Log*                       _log;
  //Task*                      _task;
    string                     _prefix;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
