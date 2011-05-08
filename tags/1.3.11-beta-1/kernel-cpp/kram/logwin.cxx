#include "precomp.h"
//#define MODULE_NAME "logwin"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann"

#if 0 // jz 12.7.01

#include <stdio.h>
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN16  &&  !defined SYSTEM_WIN16DLL  ||  defined SYSTEM_WIN32

#include <errno.h>
#include <time.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../kram/logwin.h"
#include "../kram/msec.h"

#if !defined WB_NOWORDBREAK
#   define WB_NOWORDBREAK    0   // für jz's Rechner
#endif

#if defined SYSTEM_WIN || defined SYSTEM_DOS
    const ios::open_mode ios_binary = ios::binary;
# else
#    if defined SYSTEM_GNU
        const ios::openmode ios_binary = (ios::openmode) 0;
#     else
        const ios::open_mode ios_binary = (ios::open_mode) 0;
#    endif
#endif

using namespace std;
namespace sos {


//----------------------------------------------------------------------------------Window_file

struct Window_file : Abs_file
{
                                Window_file             ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  protected:
    void                        put_record              ( const Const_area& );

  private:
    Fill_zero                  _zero_;
    Trace_window               _window;
    ostream*                   _ostream;
};

//-----------------------------------------------------------------------------Window_file_type

struct Window_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "window"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Window_file> f = SOS_NEW_PTR( Window_file() );
        return +f;
    }
};

const Window_file_type         _window_file_type;
const Abs_file_type&            window_file_type = _window_file_type;

// -------------------------------------------------------- Edit_streambuf::Edit_streambuf
#if defined LOGWIN_USE_STARVIEW

Edit_streambuf::Edit_streambuf( MultiLineEdit* edit_ptr, Trace_window* t )
:
    _trace_window( t ),
    _edit_ptr(edit_ptr),
    _new_line(true),
    _with_time_prefix(false) // Time Prefix klappt nicht
{
}

#else

Edit_streambuf::Edit_streambuf( Trace_window* t )
:
    _trace_window( t ),
    _new_line(true),
    _with_time_prefix(false) // Time Prefix klappt nicht
{
}

#endif
// -------------------------------------------------------- Edit_streambuf::~Edit_streambuf

Edit_streambuf::~Edit_streambuf()
{
}

//------------------------------------------------------------------Edit_streambuf::sync

int _Cdecl Edit_streambuf::sync()
{
    setg( 0, 0, 0 );
    return overflow();
}

//-------------------------------------------------------------Edit_streambuf::underflow

int _Cdecl Edit_streambuf::underflow()
{
    return EOF;
}

//--------------------------------------------------------------Edit_streambuf::overflow

#define MAX_EDIT_CONTROL_SIZE 32768

int _Cdecl Edit_streambuf::overflow( int b )
{
    char  line [ trace_buffer_size+30 ];
    char* l     = line;
    char* l_end = line + sizeof line - 5;
    char* p     = pbase();
    char* p_end = pptr();

    if( _new_line && with_time_prefix() ) {

#       if defined USE_GETTIMEOFDAY
            extern int gettimeofday(struct timeval *tp);
            gettimeofday( &_time_stamp );
#        else
            _time_stamp = time( 0 );
#       endif
        _elapsed_msec = elapsed_msec();     // (Zeit für das erste LOG-Zeichen geht mit ein)
    }
    if( b != EOF ) {
        if( !p_end ) {
            _buffer[ 0 ] = b;
            setp( _buffer, _buffer + trace_buffer_size );
            pbump(1);
            return 0;
        } else {
            *p_end++ = b;
        }
    }

    while( p < p_end )
    {
        if( _new_line )  new_line( &l );

        char* n = (char*)memchr( p, '\n', p_end - p );
        int len  = min( ( n? n : p_end ) - p, l_end - l );
        memcpy( l, p, len );
        p += len;
        l += len;

        if( n ) {
#           if defined SYSTEM_WIN
                *l++ = '\r';
#           endif
            *l++ = '\n';
            *p++;
            _new_line = true;
        }

        if( l >= l_end || p == p_end || _new_line )
        {
            *l++ = '\0';

#           if defined LOGWIN_USE_STARVIEW
                String text = _edit_ptr->GetText();
                String str( line );
                String prefix( "" );

                if( _trace_window->_f ) {
                    *_trace_window->_f << c_str(str);
                }

                if ( text.Len() + str.Len() > MAX_EDIT_CONTROL_SIZE )
                {
                    text.Erase( 0, min( MAX_EDIT_CONTROL_SIZE / 3, text.Len() ) ); // 1/3 löschen
                    prefix = "[Zeilen gelöscht]\r\n\r\n";
                }

                text.Insert( prefix, 0 );
                text.Insert( str );
                _edit_ptr->SetText( text );
                _edit_ptr->ChangeSelection( Selection(SELECTION_MAX) ); // Cursor ans Ende setzen
#           else
#               if defined SYSTEM_UNIX || defined SYSTEM_WIN32
                    cout << line;
#               else
#                   error Weder StarView noch cout verfügbar
#               endif
#           endif

            l = line;
        }
    }

    if( _new_line && with_time_prefix() ) {
        _last_elapsed_msec = elapsed_msec();   // oder _elapsed_time, mißt LOG() mit
    }

    setp( 0, 0 );

    return 0;
}

//--------------------------------------------------------------Edit_streambuf::new_line

void Edit_streambuf::new_line( char** l_ptr )
{
    _new_line = false;
    char* l = *l_ptr;


  if ( with_time_prefix() )
  {
#   if defined USE_GETTIMEOFDAY
        strftime( l, 9+1, "%T", localtime( &_time_stamp.tv_sec ) );
        l += 8;
        sprintf( l, ".%l04", (long)_time_stamp.tv_usec / 100 );
        l += 5;
#    else
        strftime( l, 9+1, "%T", localtime( &_time_stamp ) );
        l += 8;
#   endif
    sprintf( l, " %4ld ",  (long)( _elapsed_msec - _last_elapsed_msec ) );
  //strcpy( l + 9, _app_name );
    l += strlen( l );
  //*l++ = ' ';
    int indent = min( sos_static_ptr()->_log_indent, 50 );
    memset( l, '.', indent );
    l += indent;
    *l_ptr = l;

    _last_elapsed_msec = _elapsed_msec;
  }
}


// ---------------------------------------------------------------------- Edit_ostream::Edit_ostream
#if defined LOGWIN_USE_STARVIEW

Edit_ostream::Edit_ostream( MultiLineEdit* edit_ptr, Trace_window* t )
:
    Edit_streambuf( edit_ptr, t ),
    ostream( (Edit_streambuf*)(this) )
{

}

#else

Edit_ostream::Edit_ostream( Trace_window* t )
:
    Edit_streambuf( t ),
    ostream( (Edit_streambuf*)(this) )
{

}

#endif

// ------------------------------------------------------------------------------- log_create
/*
ostream* log_create( MultiLineEdit* _edit_ptr )
{
    return new Edit_ostream( _edit_ptr );
}
*/
// -------------------------------------------------------------------- Trace_window::Trace_window
#if defined LOGWIN_USE_STARVIEW

Trace_window::Trace_window( Window* parent, WinBits bits )
:
    WorkWindow( parent, bits ),
    _f(0),
    _edit( this, WB_READONLY|WB_HSCROLL|WB_VSCROLL|WB_HIDE|WB_BORDER|WB_NOWORDBREAK ),
    _ostream( &_edit, this )
{
    MapMode old_map_mode = ChangeMapMode( MapMode( MAP_POINT ) );
    Sos_string fontname = read_profile_string( "", "trace-window", "font" );
    if ( fontname == "" ) fontname = "Arial";
    uint fontheight = read_profile_uint( "", "trace-window", "fontheight" );
    if( fontheight <= 0 ) fontheight = 12;
    _edit.ChangeFont( Font( c_str( fontname ), Size(0,fontheight) ) );
    ChangeMapMode( old_map_mode );
    ChangeOutputSizePixel( Size( 300, 200 ) );
    _edit.Show();
}

#else

Trace_window::Trace_window()
:
    _f(0),
    _ostream( this )
{
}

#endif
// -------------------------------------------------------------------- Trace_window::~Trace_window

Trace_window::~Trace_window()
{
    if ( log_ptr == &_ostream )  log_ptr = 0;
}

// -------------------------------------------------------------------- Trace_window::file

void Trace_window::file( const char* filename )
{
    _file.open( filename, ios::out | ios::trunc | ios_binary );
    if( _file.fail() )  throw_errno( errno, "Trace_window::file" );  // js 6.11.96
    _f = &_file;
}

// -------------------------------------------------------------------- Trace_window::Resize
#if defined LOGWIN_USE_STARVIEW

void Trace_window::Resize()
{
    Size size = GetOutputSizePixel();
    _edit.ChangeSizePixel( size );
    _ostream.flush();
}

#endif
//---------------------------------------------------------------------Window_file::Window_file

Window_file::Window_file()
:
    _zero_(this+1)

#   if defined LOGWIN_USE_STARVIEW
        , _window( NULL, WB_APP|WB_STDWORK|WB_SIZEMOVE|WB_MINMAX )
#   endif
{
    _ostream = _window.ostream_ptr();

#   if defined LOGWIN_USE_STARVIEW
        _window.Show();
#   endif
}

//----------------------------------------------------------------------------Window_file::open

void Window_file::open( const char* fn, Open_mode open_mode, const File_spec& )
{
    if( open_mode & in )  throw_xc( "SOS-WINDOW-WRITEONLY" );
}

//---------------------------------------------------------------------------Window_file::close

void Window_file::close( Close_mode )
{
    show_msg( "Weiter mit OK" );
}

//----------------------------------------------------------------------Window_file::put_record

void Window_file::put_record( const Const_area& record )
{
    *_ostream << record << '\n';
}

} //namespace sos

#endif

#endif
