// logwin.h

#ifndef __LOGWIN_H
#define __LOGWIN_H

#if defined SYSTEM_STARVIEW  &&  defined SYSTEM_WIN 
#   define LOGWIN_USE_STARVIEW
#endif

#include <time.h>

#if defined LOGWIN_USE_STARVIEW  &&  !defined _SV_HXX
#   include <sv.hxx> // iostream wird von sv.hxx reingezogen
#endif

#include <windows.h>

using namespace std;
namespace sos
{

// --- struct Edit_streambuf --------------------------------------------------------------------

const int trace_buffer_size = 256;

struct Trace_window;


struct Edit_streambuf : streambuf
// kopiert aus log.cxx (Mswin_debug_streambuf), evtl. Integration von beiden!
{
#   if defined LOGWIN_USE_STARVIEW
                                Edit_streambuf          ( MultiLineEdit* edit_ptr, Trace_window* );
        MultiLineEdit*         _edit_ptr;
#   else
                                Edit_streambuf          ( Trace_window* );
#   endif
                               ~Edit_streambuf          ();

    virtual int _Cdecl          sync                    ();
    virtual int _Cdecl          underflow               ();
    virtual int _Cdecl          overflow                ( int = EOF );
    void                        new_line                ( char** );

    DECLARE_PUBLIC_MEMBER( Bool, with_time_prefix )

  private:
#   if defined USE_GETTIMEOFDAY
        struct timeval         _time_stamp;
#    else
        time_t                 _time_stamp;
#   endif

    Trace_window*              _trace_window;
    uint4                      _last_elapsed_msec;
    uint4                      _elapsed_msec;
    Bool                       _new_line;
    char                       _buffer [ trace_buffer_size+2 ];
};


// --- struct Edit_ostream -----------------------------------------------------------------------

struct Edit_ostream : Edit_streambuf, ostream
{
#if defined LOGWIN_USE_STARVIEW
                    Edit_ostream( MultiLineEdit* edit_ptr, Trace_window* );
#else
                    Edit_ostream( Trace_window* );
#endif
};


// --- struct TraceWindow -------------------------------------------

#if defined LOGWIN_USE_STARVIEW

struct Trace_window : WorkWindow
{
                                Trace_window            ( Window* = NULL, WinBits = WB_STDWORK );
                               ~Trace_window            ();

    virtual void                Resize                  ();
    virtual BOOL                Close                   () { if ( pApp->GetAppWindow() == (WorkWindow*)this ) pApp->Quit(); /*else Hide()*/; return TRUE; }
    void                        clear                   () { _ostream.flush(); _edit.SetText(""); }

    MultiLineEdit               _edit;

#else

struct Trace_window 
{
                                Trace_window            ();
                               ~Trace_window            ();

    BOOL                        Close                   ()          { return TRUE ; }
    void                        clear                   () { _ostream.flush(); }

#endif

    ostream*                    ostream_ptr             () { return &_ostream; }
    operator                    ostream&                () { return _ostream; }
    void                        file                    ( const char* );

 private:
    friend                      struct Edit_streambuf;

    ofstream                    _file;
    ostream*                    _f;
    Edit_ostream                _ostream;
};

/*
struct Auto_trace_window : Trace_window, Sos_self_deleting
{
                                Auto_trace_window() : Trace_window(), Abs_file(), Sos_self_deleting() {}
    virtual BOOL                Close() { if( TraceWindow::Close() ) { _anchor_ptr = NULL; return TRUE; } else return FALSE; }

 private:
    Sos_ptr<Auto_trace_window>  _anchor_ptr;
};
*/

//ostream* log_create( MultiLineEdit* _edit_ptr );

} //namespace sos

#endif
