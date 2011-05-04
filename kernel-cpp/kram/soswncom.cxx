#include <precomp.h>
#define MODULE_NAME "soswncom"
#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#if 0

#include <sysdep.h>
#if defined SYSTEM_WIN          // Modul nur für MS-Windows

#define STRICT
#include <windows.h>

#include <sos.h>
#include <xception.h>
#include <log.h>
#include <sosobj.h>
#include <sosmsg.h>
#include <sosfact.h>
#include <soswnmsg.h>
#include <sosarray.h>
#include <soswncom.h>

const int2 check_value = 0x7A6A;

//-------------------------------------------------------------------------Sos_mswin_comm_descr

struct Sos_mswin_comm_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "window_com"; }

    Sos_object_ptr create( Create_msg* m, int /*subtype*/, const File_spec& ) const
    {
        Sos_ptr<Sos_mswin_comm> o = SOS_NEW_PTR( Sos_mswin_comm( m->source_ptr() ) );
        return +o;
    }
};

const Sos_mswin_comm_descr    _sos_mswin_comm_descr;
extern const Sos_object_descr& sos_mswin_comm_descr = _sos_mswin_comm_descr;

//----------------------------------------------------------------------------Mswin_comm_buffer

struct Mswin_comm_buffer
{
                     Mswin_comm_buffer() : _ref_count ( 0 ), _check_value( check_value ), _length(0) {}

    int2            _ref_count;
    int2            _check_value;
    uint4           _length;
    Byte            _data [ 1 ];

    static uint                 data_offset             ()                  { return sizeof (Mswin_comm_buffer) - 1; }

    void*                       operator new            ( size_t s )
    {
        HGLOBAL  h = GlobalAlloc( s + sizeof (HGLOBAL), GMEM_SHARE | GMEM_FIXED );
        HGLOBAL* p = (HGLOBAL*)GlobalLock( h );
        *(HGLOBAL*)p = h;
        return p+1;
    }

    void                        operator delete         ( void* p, size_t )  { GlobalFree( *(((HGLOBAL*)p)-1) ); }
};


/*
struct Sos_mswin_comm_manager : Sos_object
{
                                Sos_mswin_comm_manager      ();
                               ~Sos_mswin_comm_manager      ();

    void                        init                        ();
    void                        add                         ( Sos_mswin_comm* );
    void                        del                         ( Sos_mswin_comm* );

    int                        _open_event_count;

  private:
    friend class                Sos_mswin_comm;

    Bool                       _initialized;
    Sos_simple_array<Sos_mswin_comm*>  _comm_array;

    HWND                       _hwnd;           // Window-Handle für spezielles Socket-msg-window
};
*/


Sos_mswin_comm::Sos_mswin_comm( Sos_object* creator )
:
    _manager_ptr ( 0 ),
    _run_mode   ( false ),
    _remote_data_ptr( 0 )
{
    _obj_client_ptr = creator;

    _hwnd = FindWindow( mswin_window_class_name(),
                        mswin_window_name()/*m->name()*/ );
    if( !_hwnd )  throw Xc( "MSWIN-FindWindow" );

    _msg = WM_USER + 0x102;
    register_msg( _msg );

    obj_created();
}


long Sos_mswin_comm::mswin_message_handler
                        ( HWND, UINT, WPARAM wParam, LPARAM lParam, Bool* processed )
{
    if( _remote_data_ptr )  return 0;
    if( _remote_hwnd != (HWND)wParam )  return 0;

    _remote_data_ptr = (Mswin_comm_buffer*) lParam;

    if( _remote_data_ptr->_check_value != check_value )  throw Xc( "mswin_comm" );

    if( _run_mode ) {
        post_request( Data_msg( obj_output_ptr(), this,
                                Const_area( _remote_data_ptr->_data,
                                            _remote_data_ptr->_length ) ) );
        *processed = true;
        return 0;
    }
/*
    if( _get ) {
        reply( Data_reply_msg( _input_ptr, this, Const_area( lParam, wParam ) ) );
        _receiver_buffer_ptr->_ref_count--;
        *processed = true;
        return 0;
    }
*/
    LOG_ERR( "Sos_mswin_comm: Unbearbeitete Botschaft\n" );
    return 0;
}



void Sos_mswin_comm::_obj_run_msg( Run_msg* m )
{
    _run_mode = true;
    // Wie terminiert das?
}

void Sos_mswin_comm::_obj_data_msg( Data_msg* m )
{
    ASSERT_VIRTUAL( _obj_data_msg );

/*
    HGLOBAL hglobal = GlobalAlloc( GMEM_SHARE | GMEM_MOVEABLE,
                                   Mswin_comm_buffer::data_offset() + m->data()->length() );
    if( !hglobal )  throw Xc( "MSWIN-GlobalAlloc" );
    Mswin_comm_buffer* p = new( GlobalLock( hglobal ) ) Mswin_comm_buffer();
    p->length = m->data().length;
    memcpy( p->_data, m->data().ptr(), m->data().length );
    GlobalUnlock( hglobal );
    BOOL ok = PostMessage( _hwnd, _msg, hwnd(), hglobal );
*/
    _input_ptr = m->source_ptr();

    auto_new_ptr( &_send_buffer_ptr );

    if( _send_buffer_ptr->_ref_count )  throw Busy_error( "soswncom" );
    _send_buffer_ptr->_ref_count = 1;

    _send_buffer_ptr->_length = m->data().length();
    memcpy( _send_buffer_ptr->_data, m->data().ptr(), m->data().length() );

    BOOL ok = PostMessage( _remote_hwnd, _msg, (WPARAM)hwnd(), (LPARAM)_send_buffer_ptr );
    if( !ok )  throw Xc( "MSWIN-PostMessage" );

    reply( Ack_msg( _input_ptr, this ) );
}


void Sos_mswin_comm::_obj_ack_msg( const Ack_msg* )
{
    ASSERT_VIRTUAL( _obj_ack_msg );

    _remote_data_ptr->_ref_count--;
    _remote_data_ptr = 0;
}


#endif
#endif
