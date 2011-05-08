#include "precomp.h"

#define __USELOCALES__      // für POSIX-strftime


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>          // O_CREAT etc.
#include <sys/stat.h>       // S_IREAD etc.

#include "sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#include "sosstrng.h"       // empty()
#include "sos.h"

#if defined __BORLANDC__ || defined _MSC_VER
#   include <io.h>
#elif defined SYSTEM_SOLARIS || defined SYSTEM_LINUX
#   include <unistd.h>
#endif

#if defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
#   include "olestd.h"            // w_as_string für Versionsinfo
#endif

#if defined SYSTEM_LINUX
#   include <sys/time.h>
#   include <sys/resource.h>
#   include <unistd.h>
#endif

#include "sosstat.h"
#include "sosctype.h"
#include "log.h"
#include "msec.h"
#include "thread_semaphore.h"
#include "version.h"

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/log.h"
#include "../zschimmer/check_compiler.h"
#include "../zschimmer/threads.h"

using namespace std;
namespace sos {


#if defined SYSTEM_LINUX
#   define USE_GETTIMEOFDAY
#endif


const int buffer_size = 300;
const int suppress_repeated = 3;    // Bei mehr als soviele gleiche Zeilen die Ausgabe unterdrücken

extern string                   sos_ini_msg             ();

//----------------------------------------------------------------------------Mswin_debug_streambuf

struct Mswin_debug_streambuf : streambuf
{
                                Mswin_debug_streambuf   ( const char* filename );
                               ~Mswin_debug_streambuf   ();

    virtual int _Cdecl          sync                    ();
    virtual int _Cdecl          underflow               ();
    virtual int _Cdecl          overflow                ( int = EOF );
    void                        get_app_name            ();
    void                        new_line                ( char** );
    void                        log_system_values       ( ostream* );

  private:

    Fill_zero                  _zero_;

#   if defined USE_GETTIMEOFDAY
        struct timeval         _time_stamp;
#   elif defined SYSTEM_MICROSOFT
        _timeb                 _time_stamp;
#    else
        time_t                 _time_stamp;
#   endif

    int64                      _last_elapsed_msec;
    int64                      _elapsed_msec;
    Bool                       _new_line;
    char                       _app_name [ 8+1+8+1 ];
    char                       _buffer [ buffer_size+2 ];
    int                        _out;
  //Thread_semaphore           _semaphore;
    string                     _last_line;
    int                        _repeated;
  //bool                       _print_pid;
};

//--------------------------------------------------------------------------------------Log_ostream

struct Log_ostream : Mswin_debug_streambuf, ostream
{
                                Log_ostream             ( const string& filename ) : Mswin_debug_streambuf( filename.c_str() ), ostream( (Mswin_debug_streambuf*)this ) {}
                               ~Log_ostream             ()  { sync(); }
};

//----------------------------------------------------------------------------------get_pid

static int get_pid()
{
#   ifdef SYSTEM_WIN
        return GetCurrentProcessId();
#    else
        return getpid();
#   endif
}

//------------------------------------------------------------------------------------------sos_log

void sos_log( const char* text )  
{ 
    LOG( text ); 
}

//----------------------------------------------------------------------------------------log_start

void log_start( const char* filename_ )
{
    const char* filename = filename_;

    if( !filename )  return;

    if( !log_ptr ) 
    {
        if( const char* gt = strchr( filename, '>' ) )
        {
            string log_categories ( filename, gt - filename );
            sos_static_ptr()->_log_categories = log_categories;
            zschimmer::static_log_categories.set_multiple( log_categories );
            filename = gt + 1;
            while( filename[0] == ' ' )  filename++;
        }

        ostream* s = 0;
        s = new Log_ostream( trim( filename ) );
        if( !s )  return;

        s->setf( ios::uppercase );
        time_t t = time(0);

        *s << "\n----------------------" << ctime( &t );

        *s << "Aufruf: ";
        for( int i = 0; i < _argc; i++ )  *s << ( _argv[i]? _argv[i] : "" ) << "  ";
      //for( int i = 0; i < _argc; i++ )  *s << quoted_string( _argv[i]? _argv[i] : "", '\'', '\\' ).c_str() << " ";
        *s << '\n';

        if( filename[0] != '+' )        // Etwas provisorisch: Der Scheduler startet Subprozesse mit -log=+scheduler.log. Dann soll der Log kleiner werden.
        {
            {
                char hostname[100];  memset( hostname, 0, sizeof hostname );
                char username[100];  memset( username, 0, sizeof username );

#               ifdef SYSTEM_WIN
                    DWORD length = sizeof hostname;
                    GetComputerName( hostname, &length );
                    length = sizeof username;
                    GetUserName( username, &length );
#                else
                    gethostname( hostname, sizeof hostname - 1 );
                    const char* u = getenv("USERNAME");;
                    if( !u )  u = getenv( "USER" );
                    strncpy( username, u? u : "", sizeof username - 1 );
#               endif
                *s << "host=" << hostname << ", user=" << username;
            }

            *s << ", pid=" << get_pid() << '\n';

            zschimmer::check_compiler( s );

            *s << "Modul ";
            try { *s  << module_filename() << "  " << file_version_info(module_filename()); } catch(...) {}
            *s << "\nProgramm ";
            try { *s << program_filename() << "  " << file_version_info(program_filename()); } catch(...) {}
            *s << "\n\n";

            *s << "Hostware " VER_PRODUCTVERSION_STR << "\n";

            print_all_modules( s );

/*
#           if defined SYSTEM_WIN32
                // Fehler und Warnungen beim Suchen der sos.ini ausgeben
                if ( !empty( ::sos::sos_ini_msg() ) ) *s << ::sos::sos_ini_msg() << '\n';
#           endif
*/
            *s << flush;
        }

        THREAD_LOCK( sos_static_ptr()->_log_lock )
        {
            sos_static_ptr()->_log_ptr = s;
            //zschimmer::Log_ptr::set_stream_and_system_mutex( &sos_static_ptr()->_log_ptr, &sos_static_ptr()->_log_lock._system_mutex );     // Kein Thread darf laufen (und Log_ptr benutzen)!
        }
        
        Z_LOG2( "zschimmer", Z_FUNCTION << "(\"" << filename << "\")   categories: " << zschimmer::static_log_categories.to_string() << "\n" );
    }
}

//-----------------------------------------------------------------------------------------log_stop

void log_stop()
{
    THREAD_LOCK( sos_static_ptr()->_log_lock )
    {
        zschimmer::Log_ptr::set_log_context( NULL );      // Kein Thread darf laufen (und Log_ptr benutzen)!
        //zschimmer::Log_ptr::set_stream_and_system_mutex( NULL, NULL );      // Kein Thread darf laufen (und Log_ptr benutzen)!

        SOS_DELETE( sos_static_ptr()->_log_ptr );
    }
}

//---------------------------------------------------------------------------------------log_indent

void log_indent( int direction )
{
    Sos_static* s = sos_static_ptr();

#   if defined SYSTEM_WIN
        int indent = (int)TlsGetValue( s->_log_context._indent_tls_index );
        indent += direction;
        TlsSetValue( s->_log_context._indent_tls_index, (void*)indent ); 
#    else
        s->_log_indent += direction;
#   endif

}

//---------------------------------------------------------Mswin_debug_streambuf::log_system_values

void Mswin_debug_streambuf::log_system_values( ostream* s )
{
    if( !s )  return;

#   if defined SYSTEM_WIN32
    {
        char buffer [ 200 ];

        MEMORYSTATUS m;
        memset( &m, 0, sizeof m );
        m.dwLength = sizeof m;
        GlobalMemoryStatus( &m );

        //char* old_locale = setlocale( LC_NUMERIC, "C" );

      //if( _print_pid )  s->write( buffer, sprintf( buffer, "%-5u", (uint)GetCurrentProcessId() ) );

        int len = sprintf( buffer, "%5u.%-4X %-.3fMB",
                           (uint)GetCurrentProcessId(),
                           (uint)GetCurrentThreadId(),
                           (double)( m.dwTotalVirtual - m.dwAvailVirtual ) / 1024 / 1024 );   // Das sollte der belegte Adressraum sein

        //setlocale( LC_NUMERIC, old_locale );

        #ifdef Z_DEBUG
        {
            static size_t last_size = 0;
            static size_t last_count = 0;
            _CrtMemState s;
            _CrtMemCheckpoint(&s);
            size_t size = s.lSizes[_NORMAL_BLOCK];
            size_t count = s.lCounts[_NORMAL_BLOCK];
            len += sprintf(buffer + len, " (%d %+7d bytes)", size, size - last_size);
            //len += sprintf(buffer + len, " (%d %+7d bytes, %d %+3d)", size, size - last_size, count, count - last_count);
            last_size = size;
            last_count = count;
        }
        #endif

        s->write( buffer, len );
    }
#   else
    {
        char buffer [ 15 + 15 + 15 + 15 + 15 ];

      //if( _print_pid )  s->write( buffer, sprintf( buffer, "%-5d", (uint)get_pid() ) );

        int len = sprintf( buffer, "%5u.%-4X", (uint)get_pid(), (uint)z::current_thread_id() );
        s->write( buffer, len );

      //int len = sprintf( buffer, "%-4X %-.3fMB", (uint)z::current_thread_id(), (double)(long)sbrk(0) / 1024 / 1024 );
      //s->write( buffer, len );
/*
        struct rusage u;

        int ret = getrusage( RUSAGE_SELF, &u );  // oder RUSAGE_CHILDREN?
        if( ret != -1 )
        {
            int len = sprintf( buffer, "%-4X %-.3fMB", 
                               (uint)z::current_thread_id(),
                               (double)( u.ru_maxrss ) * ( getpagesize() / 1024 ) / 1024 );        // maximum resident set size
            s->write( buffer, len );
        }
*/
    }
#   endif
}

//---------------------------------------------------------------------------Log_indent::Log_indent

Log_indent::Log_indent( Sos_static* s ) 
{ 
    _static_ptr = s; 

    log_indent( +1 );
}

//--------------------------------------------------------------------------Log_indent::~Log_indent

Log_indent::~Log_indent()
{ 
    log_indent( -1 );
}

//-------------------------------------------------Mswin_debug_streambuf::Mswin_debug_streambuf

Mswin_debug_streambuf::Mswin_debug_streambuf( const char* filename )
:
    _zero_(this+1),
    _new_line ( true ),
    _last_elapsed_msec( elapsed_msec() ),
    _out ( -1 )
  //_semaphore( "log", Thread_semaphore::kind_recursive_dont_log )
{
  //_semaphore.set_name( "log" );
    _app_name[ 0 ] = '\0';
    int oflags = 0;

#   if !defined SYSTEM_WIN
        const char* fn = empty( filename )? "/tmp/log" : filename;
#    else
        oflags |= _O_NOINHERIT;
        const char* fn = filename;
        if( !empty( filename )
         && ( strcmp( filename, "*dbwin" ) != 0  &&  strcmp( filename, "*debug" ) != 0 ) )
#   endif
    {
        //string filename2 = zschimmer::replace_regex( fn, "\\$PID[[:>:]]", as_string(get_pid()) );
        string filename2 = zschimmer::replace_regex_ext( fn, "\\$PID([^A-Za-z0-9_]|$)", as_string(get_pid()) + "\\1" );

        int o_trunc = O_TRUNC;
        if( filename2[0] == '+' )  o_trunc = 0,  filename2.erase( 0, 1 ); //, _print_pid = true;
        _out = open( filename2.c_str(), O_APPEND | O_WRONLY | O_BINARY | O_CREAT | o_trunc | oflags, S_IREAD_all | S_IWRITE_all );

        if( _out != -1 )
            sos_static_ptr()->_log_filename = filename2;
        else
        {
            char buffer [ 100 ];
            int errn = errno;
            strerror_s( buffer, sizeof buffer, errn );
            zschimmer::String_stream msg;
            msg << "Could not open log " << filename2 << ": ERRNO-" << errn << " " << buffer << "\n";
#           ifdef Z_WINDOWS
                OutputDebugString( msg.to_string().c_str() );
#           endif
            fprintf( stderr, "%s", msg.to_string().c_str() );
        }
    }
    //if ( out.fail() ) throw Xc( "???" );
}

//-------------------------------------------------Mswin_debug_streambuf::Mswin_debug_streambuf

Mswin_debug_streambuf::~Mswin_debug_streambuf()
{
    if( _out != -1 ) 
    {
        close( _out );  
        _out = -1;
        sos_static_ptr()->_log_filename = "";
    }
}

//------------------------------------------------------------------Mswin_debug_streambuf::sync

int Mswin_debug_streambuf::sync()
{
  //THREAD_LOCK( _semaphore )
    THREAD_LOCK( sos_static_ptr()->_log_lock )
    {
        setg( 0, 0, 0 );
        return overflow();
    }

    return 0;
}

//-------------------------------------------------------------Mswin_debug_streambuf::underflow

int _Cdecl Mswin_debug_streambuf::underflow()
{
    return EOF;
}

//--------------------------------------------------------------Mswin_debug_streambuf::overflow

int _Cdecl Mswin_debug_streambuf::overflow( int b )
{
  //THREAD_LOCK( _semaphore )
    THREAD_LOCK( sos_static_ptr()->_log_lock )
    {
        char  line [ buffer_size + 50 ];
        int   text_begin = 0;               // Hier beginnt der eigentliche Text (nach den Spalten für Zeit etc.)
        char* l     = line;
        char* l_end = line + sizeof line - 5;
        char* p     = pbase();
        char* p_end = pptr();

        if( _new_line ) 
        {
#           if defined USE_GETTIMEOFDAY
                gettimeofday( &_time_stamp, NULL );
#           elif defined SYSTEM_MICROSOFT
                _ftime( &_time_stamp );
#           else
                _time_stamp = time( 0 );
#           endif

            _elapsed_msec = elapsed_msec();     // (Zeit für das erste LOG-Zeichen geht mit ein)
        }

        if( b != EOF ) {
            if( !p_end ) {
                _buffer[ 0 ] = b;
                setp( _buffer, _buffer + buffer_size );
                pbump(1);
                return 0;
            } else {
                *p_end++ = b;
            }
        }

        if( p )     // Zum erstmal 5.4.2002 aufgetreten, bei ~Sos_static
        {
            while( p < p_end )
            {
                if( _new_line )  new_line( &l ),  text_begin = l - line;

                char* n = (char*)memchr( p, '\n', p_end - p );
                int len  = min( ( n? n : p_end ) - p, l_end - l );
                memcpy( l, p, len );
                p += len;

                if( len > 0  &&  l[ len-1 ] == '\r' )  len--;
                l += len;

                if( n ) {
#                   if defined SYSTEM_WIN
                        *l++ = '\r';
#                   endif
                    *l++ = '\n';
                    p++;
                    _new_line = true;
                }

                if( l >= l_end || p == p_end || _new_line )
                {
                    string      line_str;
                    const char* line_p = line;
                    int         len    = l - line;

                    *l++ = '\0';

                    if( strstr( line, "password=" ) )
                    {
                        line_str = zschimmer::remove_password( string(line,len), "?" );
                        line_p = line_str.c_str();
                        len = line_str.length();
                    }

                    if( strncmp( _last_line.c_str(), line_p + text_begin, len - text_begin ) == 0 )  
                    {
                        _repeated++;
                    }
                    else
                    {
                        if( _repeated >= suppress_repeated )
                        {
                            string str = as_string(_repeated-suppress_repeated+1) + " mal)" Z_NL;
                            if( _out != -1 )  write( _out, str.c_str(), str.length() );
                        }
                        _repeated = 0;
                    }


#                   if defined SYSTEM_WIN
#                       if defined _DEBUG
                            OutputDebugString( line_p );
#                        else
                            if( !_out )  OutputDebugString( line_p ); else
#                       endif
#                   endif
                        if( _repeated < suppress_repeated )
                        {
                            if( _out != -1 )  write( _out, line_p, len );
                            _last_line = string( line_p + text_begin, max( 0, len - text_begin ) );
                        }
                        else
                        if( _repeated == suppress_repeated )
                        {
                            string str = string( line_p, text_begin ) + "(Letzte Zeile wiederholt sich ";
                            if( _out != -1 )  write( _out, str.c_str(), str.length() );
                        }

                    l = line;
                }
            }
        }

        //if( _new_line ) {
        //    _last_elapsed_msec = _elapsed_msec;
        //}

        setp( 0, 0 );
    }

    return 0;
}

//--------------------------------------------------------------Mswin_debug_streambuf::get_app_name

void Mswin_debug_streambuf::get_app_name()
{
#   if defined SYSTEM_WIN

        if( !_app_name[ 0 ] )
        {
            HINSTANCE hInst = NULL;//_hinstance;

            HANDLE htask = GetCurrentProcess();

            if( htask ) {
                char path     [ MAXPATH+1 ];
                //char app_name [ MAXFILE+1 ];
                memset( path, 0, sizeof path );
                GetModuleFileName( hInst, (LPSTR)path, sizeof path );
                //fnsplit( path, 0, 0, app_name, 0 );
                path[ sizeof path - 1 ] = '\0';
                const char* p = path + strlen( path );
                const char* q = p;
                while( p > path  &&  p[-1] != '/'  &&  p[-1] != '\\'  &&  p[-1] != ':' )  p--;
                while( q > p  &&  q[-1] != '.' )  q--;
                if( q > p  &&  q[-1] == '.' ) q--;
                int l = min( sizeof _app_name - 1, uint( q - p ) );
                memcpy( _app_name, p, l );
                _app_name[ l ] = '\0';
                //sprintf( _app_name, "%04X ", (uint)htask );
                //strncpy( _app_name + 5, app_name, sizeof _app_name - 5 - 1 );
                //_app_name[ sizeof _app_name - 1 ] = '\0';
            }
            else
            {
                _app_name[ 0 ] = '?';
                _app_name[ 1 ] = '\0';
            }
        }
#     else  // Linux
        //sprintf( _app_name, "%u", (uint)getpid() );
        _app_name[0] = '\0';
#    endif
}

//------------------------------------------------------------------Mswin_debug_streambuf::new_line

void Mswin_debug_streambuf::new_line( char** l_ptr )
{
    _new_line = false;
    char* l = *l_ptr;

    get_app_name();

#   if defined USE_GETTIMEOFDAY
        strftime( l, 3+8+1+1, "%d %T", localtime( &_time_stamp.tv_sec ) );
        l += 3+8;
        sprintf( l, ".%03ld", (long)_time_stamp.tv_usec / 1000 );
        l += 1+3;
        sprintf( l, " %s %4ld ", _app_name, (long)( _elapsed_msec - _last_elapsed_msec ) );
#    elif defined SYSTEM_MICROSOFT
        strftime( l, 3+8+1+1, "%d %H:%M:%S", localtime( &_time_stamp.time ) );
        sprintf( l+11, ".%03d", (int)_time_stamp.millitm );
        l += 3+8+4;
        sprintf( l, " %s %4ld ", _app_name, (long)( _elapsed_msec - _last_elapsed_msec ) );
#    else
        strftime( l, 3+8+1+1, "%d %T", localtime( &_time_stamp ) );
        l += 3+8;
        sprintf( l, " %s %4ld ", _app_name, (long)( _elapsed_msec - _last_elapsed_msec ) );
#   endif

    l += strlen( l );

    {
        ostrstream s ( l, 75 );
        log_system_values( &s );
        l += s.pcount();
        *l++ = ' ';
    }

#   if defined SYSTEM_WIN
        int indent = (int)TlsGetValue( sos_static_ptr()->_log_context._indent_tls_index );
#    else
        int indent = sos_static_ptr()->_log_indent;
#   endif

    indent = min( indent, 50 );
    memset( l, '.', indent );
    l += indent;
    *l_ptr = l;

    _last_elapsed_msec = _elapsed_msec;
}

//----------------------------------------------------------------------------------------log_write

int Log_ptr::log_write( const char* text, int length )
{
    if( ostream* log = sos_static_ptr()->_log_ptr )
    {
        if( text != NULL )
        {
            log->write( text, length );
        }
        else
        {
            if( length == 1 )  log->flush();
        }
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
