#include "precomp.h"
//#define MODULE_NAME "soswindw"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sysdep.h"
#if 0 //defined SYSTEM_WIN          // Modul nur für MS-Windows

#define STRICT
#include <windows.h>

#include <sos.h>
#include <xception.h>
#include <sosobj.h>
#include <log.h>
#include <soslist.h>
#include <sosmswin.h>
#include <soswnmsg.h>

using namespace std;
namespace sos {

extern const char* sos_window_class_name = "SOS_Berlin_window_class";  // Fest für alle Zeiten!!
extern const char* sos_window_name       = "Sos_internal_window";

struct Mswin_msg_window_manager : Sos_object
{
    struct Entry
    {
                                   Entry( Has_mswin_message_handler* p, uint m ) : handler_ptr( p ), msg( m ) {}
        Has_mswin_message_handler* handler_ptr;
        uint                       msg;
    };

    typedef Sos_simple_list_node<Entry> Msg_handler_node;

                                Mswin_msg_window_manager    ();
                               ~Mswin_msg_window_manager    ();

    void                        init                        ();
    void                        add                         ( Has_mswin_message_handler*, uint msg );
    void                        del                         ( Has_mswin_message_handler* );

    LRESULT                     mswin_message_handler       ( HWND, UINT, WPARAM, LPARAM, Bool* );

    int                        _open_event_count;

  private:
    friend class                Has_mswin_message_handler;
    Msg_handler_node*          _msg_handler_list;

    HWND                       _hwnd;           // Window-Handle für spezielles Socket-msg-window
};

//------------------------------------------------------------------------mswin_message_handler

LRESULT __export __pascal mswin_message_handler( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    LRESULT                   result    = 0;

    try {
        Bool                      processed = false;
        Mswin_msg_window_manager* m         = (Mswin_msg_window_manager*)+sos_static_ptr()->_mswin_msg_window_manager_ptr;

        if( m )           result = m->mswin_message_handler( hwnd, msg, wParam, lParam, &processed );
        if( !processed )  result = DefWindowProc( hwnd, msg, wParam, lParam );
    }
    catch( const Xc& x )
    {
        SHOW_ERR( "Exception wird ignoriert: " << x );
    }
    catch( ... )
    {
        SHOW_ERR( "Exception wird ignoriert: " << Xc( "UNKNOWN" ) );
    }

    return result;
}

//----------------------------------------------Mswin_msg_window_manager::mswin_message_handler

LRESULT Mswin_msg_window_manager::mswin_message_handler
                       ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, Bool* processed )
{
    LOGI( "Mswin_msg_window_manager::mswin_window_message( " << msg << ", " << wParam << ", " << lParam << ")\n" << dec );

    LRESULT result    = 0;

    Msg_handler_node* n = _msg_handler_list;

    while( !empty( n ) ) {
        if( n->head().msg == msg )  break;
        n = n->tail();
    }

    if( !empty( n ) ) {
        n->head().handler_ptr->mswin_message_handler( hwnd, msg, wParam, lParam, processed );
        if( !*processed )  LOG( "... nicht verarbeitet\n" );
    } else {
        LOG( "... unbekannte Botschaft\n" );
    }

    return result;
}

//----------------------------------------Has_mswin_message_handler::~Has_mswin_message_handler

Has_mswin_message_handler::Has_mswin_message_handler( uint msg )
{
    register_msg( msg );
}

//----------------------------------------Has_mswin_message_handler::~Has_mswin_message_handler

Has_mswin_message_handler::~Has_mswin_message_handler()
{
    Mswin_msg_window_manager* m = (Mswin_msg_window_manager*)+sos_static_ptr()->_mswin_msg_window_manager_ptr;

    if( m ) {
        m->del( this );
    }/* else {
        SHOW_ERR( "mswin_msg_window_manager_ptr == 0" );
    }*/
}

//----------------------------------------Has_mswin_message_handler::~Has_mswin_message_handler

void Has_mswin_message_handler::register_msg( uint msg )
{
    if( !sos_static_ptr()->_mswin_msg_window_manager_ptr ) {
        Sos_ptr<Mswin_msg_window_manager> m = SOS_NEW_PTR( Mswin_msg_window_manager );
        sos_static_ptr()->_mswin_msg_window_manager_ptr = m;
    }

    ((Mswin_msg_window_manager*)+sos_static_ptr()->_mswin_msg_window_manager_ptr)->add( this, msg );
}

//--------------------------------------------------------------Has_mswin_message_handler::hwnd

HWND Has_mswin_message_handler::hwnd() const
{
    return ((Mswin_msg_window_manager*)+sos_static_ptr()->_mswin_msg_window_manager_ptr)->_hwnd;
}

//-------------------------------------------Has_mswin_message_handler::mswin_window_class_name

const char* Has_mswin_message_handler::mswin_window_class_name()
{
    return sos_window_class_name;
}

//-------------------------------------------------Has_mswin_message_handler::mswin_window_name

const char* Has_mswin_message_handler::mswin_window_name()
{
    return sos_window_name;
}

//-------------------------------------------Mswin_msg_window_manager::Mswin_msg_window_manager

#if defined JS_TEST
extern HWND _app_window_handle;
#endif


Mswin_msg_window_manager::Mswin_msg_window_manager()
:
    _hwnd ( 0 )
{
    try {
        int         rc;
        WNDCLASS    wc;

        wc.style         = 0;
        wc.lpfnWndProc   = ::mswin_message_handler;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = Mswin::hinstance();
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = sos_window_class_name;

        rc = RegisterClass( &wc );
        if( !rc )  throw Xc( "MSWIN-RegisterClass" );

        _hwnd = CreateWindow( sos_window_class_name,    // Klassenname
                              sos_window_name,          // Fenstername
                              DS_NOIDLEMSG | WS_DISABLED,// Style
                              0,                        // x
                              0,                        // y
                              1,                        // Breite
                              1,                        // Höhe
#if defined JS_TEST
                              _app_window_handle,       // parent window
#else
                              HWND_DESKTOP,             // parent window
#endif
                              NULL,                     // hmenu
                              Mswin::hinstance(),       // hinst
                              NULL );                   // lpvParam

        if( !_hwnd )  throw Xc( "MSWIN-CreateWindow" );
        LOG( "Mswin_msg_window_manager::_hwnd = " << hex << (uint)_hwnd << dec << '\n' );

    }
    catch(...)
    {
        DestroyWindow( _hwnd );
        UnregisterClass( sos_window_class_name, Mswin::hinstance() );
        throw;
    }
}

//------------------------------------------Mswin_msg_window_manager::~Mswin_msg_window_manager

Mswin_msg_window_manager::~Mswin_msg_window_manager()
{
    LOGI( "~Mswin_msg_window_manager\n" );

    int len = length( _msg_handler_list );
    if( len ) {
        SHOW_ERR( len << " Has_mswin_message_handler sind noch eingetragen" );
    }

    BOOL ok;
    LOG( "DestroyWindow()\n" );
    ok = DestroyWindow( _hwnd );
    if( !ok )  LOG( "DestroyWindow() fehlerhaft\n" );

    LOG( "UnregisterClass()\n" );
    ok = UnregisterClass( sos_window_class_name, Mswin::hinstance() );
    if( !ok )  LOG( "UnregisterClass() fehlerhaft\n" );
}

//----------------------------------------------------------------Mswin_msg_window_manager::add

void Mswin_msg_window_manager::add( Has_mswin_message_handler* handler_ptr, uint msg )
{
    _msg_handler_list = new Msg_handler_node( Msg_handler_node( Entry( handler_ptr, msg ),
                                                                _msg_handler_list ) );
};

//----------------------------------------------------------------Mswin_msg_window_manager::del

void Mswin_msg_window_manager::del( Has_mswin_message_handler* handler_ptr )
{
    Msg_handler_node** node_ptr = &_msg_handler_list;

    while( !empty( *node_ptr ) ) {
        if( (*node_ptr)->head().handler_ptr == handler_ptr ) {
            delete_node( node_ptr );
        } else {
            node_ptr = (Msg_handler_node**)&(*node_ptr)->_tail;
        }
    }

    if( empty( _msg_handler_list ) ) {
        DELETE( sos_static_ptr()->_mswin_msg_window_manager_ptr );
    }
}

} //namespace sos
#endif
