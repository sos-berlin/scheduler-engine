// $Id: log.h 13693 2008-10-02 14:15:45Z jz $

#ifndef ZSCHIMMER_LOG_H
#define ZSCHIMMER_LOG_H

#include "mutex.h"
#include <iostream>
#include "log_categories.h"

namespace zschimmer {

namespace io
{
    struct Char_sequence;
}

namespace com {}

struct Log_categories;
struct Log_categories_cache;

//-------------------------------------------------------------------------------------------------

extern Log_categories               static_log_categories;
extern Log_categories_cache         static_log_categories_cache;

//------------------------------------------------------------------------------------------Log_ptr

struct Log_ptr
{
    struct Log_streambuf : std::streambuf
    {
                                    Log_streambuf           ();
                                 //~Log_streambuf           ()                                      { sync(); }      // Kann bei atexit() abstürzen

        virtual int                 sync                    ();
        virtual int                 underflow               ();
        virtual int                 overflow                ( int );

        char                       _buffer                  [ 1024 ];
    };

    struct Log_ostream : ostream
    {
                                    Log_ostream             ()                                      : ostream( &_streambuf ) {}

        Log_streambuf              _streambuf;
    };



    struct Indent
    {
                                Indent                      ()                                      { indent(); }
                               ~Indent                      ();         

      private:
        void                    indent                      ();         
        void                    indent_                     ( int );         
    };


                                Log_ptr                     ();
                                Log_ptr                     ( const char*          category, const char* function = NULL );
                                Log_ptr                     ( const string&        category, const char* function = NULL );
                                Log_ptr                     ( Cached_log_category& category, const char* function = NULL );
                               ~Log_ptr                     ();


  //static int                  log_write                   ( const char*, int length );
    static bool                 logging                     ()                                      { return static_log_context_ptr != NULL; }
    static void                 disable_logging             ()                                      { static_stream_ptr = NULL; }   // Nach fork() aufzurufen, weil Mutex gesperrt sein kann
    static void                 set_log_context             ( Log_context** );
    static Log_context**        get_log_context             ()                                      { return static_log_context_ptr; }  // Für hostole.cxx
  //static void                 set_stream_and_system_mutex ( ostream**, System_mutex* );
  //static void                 get_stream_and_system_mutex ( ostream***, System_mutex** );
  //static void                 set_indent_tls_index        ( uint tls_index )                      { static_log_context._indent_tls_index = tls_index; }
    
                                operator ostream*           ()                                      { return _stream; }
  //ostream&                    operator *                  ()                                      { return *_stream; }
    ostream*                    operator ->                 ()                                      { return _stream; }
    void                        flush                       () const                                { if( _stream )  _stream->flush(); }  
    void                        log_function                ( const char* );


    template<class T>
    Log_ptr& operator<< ( const T& t )                    
    { 
        using namespace com;  // Für VARIANT und BSTR
        
        if( _stream )  
        {
            *_stream << t;
        }

        return *this; 
    }


    ostream* const             _stream;

  private:
    static Log_context**        static_log_context_ptr;
    static Log_context*         static_no_log_context;
  //static Log_context          static_log_context;
  //static System_mutex*        static_system_mutex_ptr;
    static ostream*             static_stream_ptr;
    static Log_ostream          static_stream;
  //static ostream**            static_hostware_stream_ptr;
  //static uint                 static_indent_tls_index;
};


#define Z_LOG( TEXT )               Z_LOG2( "", TEXT )
#define Z_LOG2( CATEGORY, TEXT )    if( zschimmer::Log_ptr __log_ptr__ = CATEGORY )  __log_ptr__.log_function( Z__PRETTY_FUNCTION__ ), *__log_ptr__._stream << TEXT;  else (void)0
#define Z_LOGI( TEXT )              Z_LOG( TEXT ); zschimmer::Log_ptr::Indent __log_indent__
#define Z_LOGI2( CATEGORY, TEXT )   Z_LOG2( CATEGORY, TEXT );  zschimmer::Log_ptr::Indent __log_indent__

//----------------------------------------------------------------------------------------Log_level

enum Log_level
{
    log_unknown= -99,
    log_none   = -10,
    log_debug9 = -9,
    log_min    = -9,         
    log_debug8 = -8,
    log_debug7 = -7,
    log_debug6 = -6,
    log_debug5 = -5,
    log_debug4 = -4,
    log_debug3 = -3,
    log_debug2 = -2,
    log_debug1 = -1,
    log_debug  = -1,
    log_info   =  0, 
    log_warn   =  1, 
    log_error  =  2,
    log_fatal  =  3,
  //log_none   =  4,        // Nicht loggen 
};

//----------------------------------------------------------------------------------------Log_level

Log_level                       make_log_level              ( const string& );
string                          name_of_log_level           ( Log_level );
inline size_t                   hash_value                  ( Log_level level )                     { return (size_t)level; }

//------------------------------------------------------------------------------------------Has_log

struct Has_log
{
                                Has_log                     ()                                      : _log_level(log_unknown) {}
    virtual                    ~Has_log                     ()                                      {}

    void                    set_log_level                   ( Log_level level )                     { _log_level = level; }
    Log_level                   log_level                   ()                                      { return _log_level; }

    void                        operator()                  ( const string& line )                  { info( line ); }
    void                        debug9                      ( const string& line )                  { log( log_debug9, line ); }
    void                        debug8                      ( const string& line )                  { log( log_debug8, line ); }
    void                        debug7                      ( const string& line )                  { log( log_debug7, line ); }
    void                        debug6                      ( const string& line )                  { log( log_debug6, line ); }
    void                        debug5                      ( const string& line )                  { log( log_debug5, line ); }
    void                        debug4                      ( const string& line )                  { log( log_debug4, line ); }
    void                        debug3                      ( const string& line )                  { log( log_debug3, line ); }
    void                        debug2                      ( const string& line )                  { log( log_debug2, line ); }
    void                        debug1                      ( const string& line )                  { log( log_debug1, line ); }
    void                        debug                       ( const string& line )                  { log( log_debug3, line ); }
    void                        info                        ( const string& line )                  { log( log_info  , line ); }
    void                        warn                        ( const string& line )                  { log( log_warn  , line ); }
    void                        error                       ( const string& line )                  { log( log_error , line ); }
    void                        fatal                       ( const string& line )                  { log( log_fatal , line ); }
    virtual void                log                         ( Log_level level, const string& line ) { log2( level, "", line ); }
    virtual void                log2                        ( Log_level, const string& prefix, const string& line, Has_log* prefix_log = NULL )  = 0;
    void                        log_file                    ( const string& filename, const string& title = "" );
    void                        log_with_prefix             ( const string& prefix, const io::Char_sequence& );;
    void                        log_with_prefix             ( const string& prefix, const string& );
    void                        log_with_title              ( const string& title, const io::Char_sequence& seq );

    Log_level                  _log_level;
};

//-------------------------------------------------------------------------------------------------

struct Delegated_log : Has_log
{
                                Delegated_log               ( Has_log* outer_log = NULL, const string& prefix = "" ) : _zero_(this+1), _log(outer_log), _prefix(prefix) {}

    void                    set_log                         ( Has_log* outer_log )                  { _log = outer_log; }
    Has_log*                    base_log                    () const                                { return _log; }

  //void                    set_log_level                   ( int level )                           { _log_level = level; }
  //int                         log_level                   ()                                      { return _log_level; }

    void                    set_prefix                      ( const string& prefix )                { _prefix = prefix; }
    string                      prefix                      ()                                      { return _prefix; }

    void                        log                         ( Log_level level, const string& line ) { log2( level, _prefix, line ); }
    void                        log2                        ( Log_level, const string& prefix, const string& line, Has_log* = NULL );


    Fill_zero                  _zero_;
    Has_log*                   _log;
    string                     _prefix;
};

//-----------------------------------------------------------------------------Log_categories_cache
// Fürs scheduler.log, static_log_categories

struct Log_categories_cache
{
                                Log_categories_cache        ();

    Cached_log_category        _async;
    Cached_log_category        _function;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


Z_DEFINE_GNU_HASH_VALUE( zschimmer, Log_level )

#endif
