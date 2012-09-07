// log.h                                                (c) SOS GmbH Berlin
//                                                          Joacim Zschimmer

#ifndef __LOG_H
#define __LOG_H

#include "../kram/sosstat.h"


namespace sos
{


extern void                     sos_log                     ( const char* );
extern void*                    log_create                  ( const char* filename = 0 );
extern void                     log_start                   ( const char* filename = 0 );
extern void                     log_stop                    ();
inline string                   log_filename                ()                              { return sos_static_ptr()->_log_filename; }
inline string                   current_log_categories      ()                              { return sos_static_ptr()->_log_categories; }


#if defined __SOSSTRNG_H
inline void log_start( const Sos_string& filename )
{
    log_start( c_str( filename ) );
}
#endif

#define _LOG( text )       do { ::sos::Log_ptr _l_; if( _l_ )  { *_l_ << text; _l_->flush(); } } while(0)

#if defined DONT_LOG
#   define VOID_LOG( text )  ((void)0)
#   define LOG( text )       do {} while(0)
#   define LOGI( text )      LOG( text ); ;
#else
#   define VOID_LOG( text )  ( log_ptr? (void)( *log_ptr << text ) : (void)0 )
#   define LOG( text )       _LOG( text )
#   define LOG_INDENT_       ::sos::Log_indent __log_indent__##__LINE__;
#   define LOGI( text )      LOG( text );  LOG_INDENT_
#endif

#define LOG_ERR( text )   _LOG( text )   // Fehler

//---------------------------------------------------------------------------........log_categories

using zschimmer::log_category_is_set;
using zschimmer::set_log_category;
using zschimmer::set_log_category_default;
using zschimmer::set_log_category_implicit;
using zschimmer::set_log_category_explicit;
using zschimmer::log_categories_as_string;

#define LOG2( CATEGORY, TEXT )   if( log_category_is_set( CATEGORY ) )  { LOG( TEXT ); }  else;  
#define LOGI2( CATEGORY, TEXT )  if( log_category_is_set( CATEGORY ) )  { LOG( TEXT ); }  LOG_INDENT_

//-------------------------------------------------------------------------------------------------

void                            log_indent                  ( int direction );      // +1 oder -1

//------------------------------------------------------------------------------------------Log_ptr

struct Log_ptr
{
                                Log_ptr                     ()                                      : _s( sos_static_ptr()->_log_ptr ) { if( _s ) sos_static_ptr()->_log_lock.enter(); }
                               ~Log_ptr                     ()                                      { if( _s ) sos_static_ptr()->_log_lock.leave(); }

                                operator ostream*           ()                                      { return _s; }
    ostream&                    operator *                  ()                                      { return *_s; }
    ostream*                    operator ->                 ()                                      { return _s; }

    static int                  log_write                   ( const char*, int length );
    static void                 set_demo_version            ( bool is_demo_version )                { static_is_demo_version = is_demo_version; }
    static bool                 is_demo_version             ()                                      { return static_is_demo_version; }


    ostream*                   _s;

  private:
    static bool                 static_is_demo_version;

};

#define log_ptr ( Log_ptr()._s )
// Damit sollte LOG() und *log_ptr << ... << ...  durch die Semaphore serialisiert sein.

//#define log_ptr ( sos::sos_static_ptr()->_log_ptr )

//---------------------------------------------------------------------------------------Log_indent

struct Log_indent
{
                                Log_indent                  ( Sos_static* s = sos::sos_static_ptr() );
                               ~Log_indent                  ();

  private:

  //static int                 _log_indent_tls;

    Sos_static* _static_ptr;
};


/*
#if defined DONT_LOG

    struct Log_block
    {
        Log_block( const char* ) {}
    };

#else

    struct Log_block
    {
        Log_block( const char* text_ptr )
        :
            _text_ptr( text_ptr ),
            _static_ptr ( sos_static_ptr() )
        {
            LOG( _text_ptr << " begin\n" );
            _static_ptr->_log_indent++;
        }

        ~Log_block()
        {
            _static_ptr->_log_indent--;
            LOG( _text_ptr << " end\n" );
        }

      private:
        Sos_static* _static_ptr;
        const char* _text_ptr;
    };

#endif
*/

} //namespace sos

#endif
