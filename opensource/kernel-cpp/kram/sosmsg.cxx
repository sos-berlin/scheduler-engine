#include "precomp.h"

//#define MODULE_NAME "sosmsg"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"

#include "../kram/sossock1.h"

#include <limits.h>


#if defined SYSTEM_WIN
#   if defined SYSTEM_STARVIEW
#       include <svwin.h>
#   elif !defined SYSTEM_MICROSOFT
#       include <windows.h>
#   endif
#endif

#include "../kram/sos.h"
#include "../kram/sosstat.h"
#include "../kram/soswin.h"
#include "../kram/sosarray.h"
#include "../kram/msec.h"
#include "../kram/log.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfield.h"
#include "../kram/stdfield.h"
#include "../kram/sossock.h"
#include "../kram/waitmsg.h"

using namespace std;
namespace sos {

//int sosmsg_dispatcher_active = 0;

//---------------------------------------------------------------------------------Sos_msg_free
/*
struct Sos_msg_free
{
                                Sos_msg_free() : _next ( this + 1 ) {}
    Sos_msg_free*              _next;
    Byte                       _x [ 64 - sizeof (Sos_msg_free*) ]; //_next ];
};

//---------------------------------------------------------------------------------sos_msg_free

Sos_msg_free  sos_msg_free [ 10 ];
Sos_msg_free* sos_msg_free_ptr = sos_msg_free;
*/
//--------------------------------------------------------------------------------Sos_msg_queue

struct Sos_msg_queue : Sos_self_deleting
{
    struct Queue_entry
    {
                                Queue_entry             ()      : _msg_ptr ( 0 )/*, _delete_msg( false )*/ {}

        Sos_ptr<Sos_msg>       _msg_ptr;
      //Bool                   _delete_msg;
    };

                                Sos_msg_queue           ();
                               ~Sos_msg_queue           ();

    void                        enqueue                 ( const Sos_ptr<Sos_msg>& );
    void                        dispatch_next_msg       ( Bool dispatch_os_msg );
    int  /*Anzahl*/             dispatch_next_msg       ( int count, const Sos_object*, Bool dispatch_os_msg = true );


  private:
  //Sos_list_node<Sos_msg*>*   _msg_list;
    int                        _queue_begin;
    int                        _queue_end;
    Sos_simple_array<Queue_entry> _msg_array;
};

//Sos_msg_queue sos_msg_queue;

DEFINE_SOS_STATIC_PTR( Sos_msg_queue )

//----------------------------------------------------------------------------Sos_owner::~Sos_owner
/*
Sos_owner::~Sos_owner()
{
    remove_possession();
}

//-----------------------------------------------------------------------------------Sos_owner::add

void Sos_owner::add( Sos_object* object_ptr )
{
};

*/
//---------------------------------------------------------------------------------------------send

void send( Sos_msg* msg_ptr )
{
    //Request_msg* request_msg_ptr = dynamic_cast<Request_msg*>( msg_ptr );
    if( msg_ptr->is_request() ) {
        request( (Request_msg*)msg_ptr );
        return;
    }
    else
    {
        reply( *(Reply_msg*)msg_ptr );
        return;
    }
}

//------------------------------------------------------------------------Sos_msg::operator new
#if 0
void* Sos_msg::operator new( size_t size )
{
//  if( size > sizeof( Sos_msg_free )
//   || sos_msg_free_ptr == sos_msg_free + NO_OF( sos_msg_free ) )
//  {
        return /*Base_class*/::operator new( size );
//  }

//  void* ptr = sos_msg_free_ptr;
//  sos_msg_free_ptr = sos_msg_free_ptr->_next;
//  return ptr;
}
#endif
//---------------------------------------------------------------------Sos_msg::operator delete
#if 0
void Sos_msg::operator delete( void* ptr, size_t size )
{
//  if( /*size > sizeof( Sos_msg_free )
//   ||*/(Byte __huge*)ptr <  (Byte __huge*)sos_msg_free
//   || (Byte __huge*)ptr >= (Byte __huge*)( sos_msg_free + NO_OF( sos_msg_free ) ) )
//  {
        /*Base_class*/::operator delete( ptr/*, size*/ );
//      return;
//  }
//
//  ((Sos_msg_free*)ptr)->_next = sos_msg_free_ptr;
//  sos_msg_free_ptr = (Sos_msg_free*)ptr;
}
#endif
//----------------------------------------------------------------------------Sos_msg::~Sos_msg

Sos_msg::~Sos_msg()
{
}

//--------------------------------------------------------------------------Sos_msg::_obj_print

void Sos_msg::_obj_print( ostream* s ) const
{
    //*s << "Sos_msg( " << *dest_ptr() << ", " << *source_ptr() << ", " << type() << " )";
    *s << "Sos_msg( " << (int)type() << " )";
}

//----------------------------------------------------------------------------Run_msg::new_copy
/*
Sos_msg* Run_msg::new_copy() const
{
    Sos_ptr<Run_msg> p = SOS_NEW_PTR( Run_msg( *this ) );
    return +p;
}
*/
//--------------------------------------------------------------------------Run_msg::_obj_print

void Run_msg::_obj_print( ostream* s ) const
{
    //*s << "Run_msg( " << *dest_ptr() << ", " << *source_ptr() << " )";
    *s << "Run_msg";
}

//-------------------------------------------------------------------------------Data_msg::new_copy
/*
Sos_msg* Data_msg::new_copy() const
{
    return new Data_msg( *this );
}
*/
//-----------------------------------------------------------------------------Data_msg::_obj_print

void Data_msg::_obj_print( ostream* s ) const
{
    //*s << "Data_msg( " << *dest_ptr() << ", " << *source_ptr() << ", len=" << data().length() << " )";
    *s << "Data_msg( len=" << data().length() << " )";
}

//--------------------------------------------------------------------------------Data_msg::key
/*
const Const_area_handle& Data_msg::key()
{
    if( !_key.length()  &&  _seek_position != (uint4)-1 ) {
        Dynamic_area key ( 4 );
        write_field( Uint4_field(), key.byte_ptr(), _seek_position );
        key.length( 4 );
        _key = key;
    }

    return _key;
}
*/
//--------------------------------------------------------------------------------Ack_msg::new_copy
/*
Sos_msg* Ack_msg::new_copy() const
{
    return new Ack_msg( *this );
}
*/
//------------------------------------------------------------------------------Ack_msg::_obj_print

void Ack_msg::_obj_print( ostream* s ) const
{
    //*s << "Ack_msg( " << *dest_ptr() << ", " << *source_ptr() << " )";
    *s << "Ack_msg";
}

//--------------------------------------------------------------------------------End_msg::new_copy
/*
Sos_msg* End_msg::new_copy() const
{
    return new End_msg( *this );
}
*/
//------------------------------------------------------------------------------End_msg::_obj_print

void End_msg::_obj_print( ostream* s ) const
{
    //*s << "End_msg( " << *dest_ptr() << ", " << *source_ptr() << " )";
    *s << "End_msg";
}

//-------------------------------------------------------------------------Cancel_msg::new_copy
/*
Sos_msg* Cancel_msg::new_copy() const
{
    return new Cancel_msg( *this );
}
*/
//-----------------------------------------------------------------------Cancel_msg::_obj_print

void Cancel_msg::_obj_print( ostream* s ) const
{
    *s << "Cancel_msg";
}

//-------------------------------------------------------------------------------Spec_msg::new_copy
/*
Sos_msg* Spec_msg::new_copy() const
{
    return new Spec_msg( *this );
}
*/
//-----------------------------------------------------------------------------Spec_msg::_obj_print

void Spec_msg::_obj_print( ostream* s ) const
{
    dest_ptr()->obj_print_spec_msg( s, *this );
//    *s << "Spec_msg( " << *dest_ptr() << ", " << *source_ptr() << ", "
//                       << spec_code() << " )";
}

//-------------------------------------------------------------------------Object_ref_msg::new_copy
/*
Sos_msg* Object_ref_msg::new_copy() const
{
    return new Object_ref_msg( *this );
}
*/
//-----------------------------------------------------------------------Object_ref_msg::_obj_print

void Object_ref_msg::_obj_print( ostream* s ) const
{
    //*s << "Object_ref_msg( " << *dest_ptr() << ", " << *source_ptr() << ", " << *object_ptr() << " )";
    *s << "Object_ref_msg( " << *object_ptr() << " )";
}

//-------------------------------------------------------------------------Create_msg::new_copy
/*
Sos_msg* Create_msg::new_copy() const
{
    return new Create_msg( *this );
}
*/
//-----------------------------------------------------------------------Create_msg::_obj_print

void Create_msg::_obj_print( ostream* s ) const
{
    //*s << "Create_msg( " << *dest_ptr() << ", " << *source_ptr() << ", " << c_str( _name ) << " )";
    *s << "Create_msg( \"" << c_str( _name ) << "\", " << _owner << " )";
}

//----------------------------------------------------------------------------Destroy_msg::new_copy
/*
Sos_msg* Destroy_msg::new_copy() const
{
    return new Destroy_msg( *this );
}
*/
//--------------------------------------------------------------------------Destroy_msg::_obj_print

void Destroy_msg::_obj_print( ostream* s ) const
{
    *s << "Destroy_msg( "/* << *dest_ptr() << ", " << *source_ptr() << ", "*/ << (int)_mode << " )";
}

//--------------------------------------------------------------------------Get_msg::_obj_print

void Get_msg::_obj_print( ostream* s ) const
{
    //*s << "Run_msg( " << *dest_ptr() << ", " << *source_ptr() << " )";
    *s << "Get_msg";
}

//------------------------------------------------------------------Prepare_to_commit_msg::new_copy
/*
Sos_msg* Prepare_to_commit_msg::new_copy() const
{
    return new Prepare_to_commit_msg( *this );
}

//--------------------------------------------------------------------------Roll_back_msg::new_copy

Sos_msg* Roll_back_msg::new_copy() const
{
    return new Roll_back_msg( *this );
}

//-----------------------------------------------------------------------------Commit_msg::new_copy

Sos_msg* Commit_msg::new_copy() const
{
    return new Commit_msg( *this );
}

//--------------------------------------------------------------------------------Get_msg::new_copy

Sos_msg* Get_msg::new_copy() const
{
    return new Get_msg( *this );
}

//-------------------------------------------------------------------------Data_reply_msg::new_copy

Sos_msg* Data_reply_msg::new_copy() const
{
    return new Data_reply_msg( *this );
}
*/
//--------------------------------------------------------------------------Data_reply_msg::key
/*
const Const_area_handle& Data_reply_msg::key()
{
    if( !_key.length()  &&  _seek_pos != -1 ) {
        Dynamic_area key ( 4 );
        write_field( Uint4_field(), key.byte_ptr(), _seek_pos );
        key.length( 4 );
        _key = key;
    }

    return _key;
}
*/
//-----------------------------------------------------------------------Data_reply_msg::_obj_print

void Data_reply_msg::_obj_print( ostream* s ) const
{
    //*s << "Data_reply_msg( " << *dest_ptr() << ", " << *source_ptr() << ", len=" << data().length() << " )";
    *s << "Data_reply_msg( len=" << data().length() << " )";
}

//------------------------------------------------------------------------------Error_msg::new_copy
/*
Sos_msg* Error_msg::new_copy() const
{
    return new Error_msg( *this );
}
*/
//----------------------------------------------------------------------------Error_msg::_obj_print

void Error_msg::_obj_print( ostream* s ) const
{
    *s << "Error_msg( " << error().code() << " )";
}

//---------------------------------------------------------------------Sos_msg_queue::Sos_msg_queue

Sos_msg_queue::Sos_msg_queue()
:
    _queue_begin ( 0 ),
    _queue_end   ( 0 )
{
    _msg_array.obj_const_name( "Sos_msg_queue::_msg_array" );
    _msg_array.last_index( 1000-1 );
}

//--------------------------------------------------------------------Sos_msg_queue::~Sos_msg_queue

Sos_msg_queue::~Sos_msg_queue()
{
}

//---------------------------------------------------------------------------Sos_msg_queue::enqueue

void Sos_msg_queue::enqueue( const Sos_ptr<Sos_msg>& m )
{
    if( !m->dest_ptr() ) {
        throw_xc( "SOS-1132", +m );
    }
    if( m->dest_ptr()->obj_ref_count() < 1 )   { Xc x ( "SOS-1151" ); x.insert( m->source_ptr() ); /* x.insert( *m ), wenn Sos_object_base */ throw x; }
    if( m->is_request() ) {
        //LOG( "post " << *m->source_ptr() << " --> " << *m->dest_ptr() << '.' << *m << '\n' );
    } else {
        //LOG( "post " << *m->dest_ptr() << '.' << *m << " <-- " << *m->source_ptr() << '\n' );
    }

    int new_queue_end = _queue_end + 1;

    if( new_queue_end > _msg_array.last_index() ) {
        new_queue_end = _msg_array.first_index();
    }

    if( new_queue_end == _queue_begin )  throw_xc( "SOS-OBJ-1009" );

    _msg_array[ _queue_end ]._msg_ptr    = (Sos_msg*)m;
  //_msg_array[ _queue_end ]._delete_msg = delete_msg;

    _queue_end = new_queue_end;
}

//-----------------------------------------------------------------Sos_msg_queue::dispatch_next_msg

void Sos_msg_queue::dispatch_next_msg( Bool dispatch_os_msg )
{
    //if( !dispatch_os_msg  &&  _queue_begin == _queue_end )  return;

    while(1)
    {
        int c = dispatch_next_msg( 9999, 0, dispatch_os_msg );
        if( c < 9999 )  break;
    }
}

//-----------------------------------------------------------------------------------_msg_queue_ptr

static Sos_msg_queue* _msg_queue_ptr()
{
    Sos_msg_queue* p = sos_static_ptr()->_msg_queue_ptr;
    if( !p ) {
        Sos_ptr<Sos_msg_queue> q = SOS_NEW( Sos_msg_queue );
        sos_static_ptr()->_msg_queue_ptr = +q;
        p = q;
    }
    return p;
}

//------------------------------------------------------------------------------------msg_queue_ptr

inline Sos_msg_queue* msg_queue_ptr()
{
    Sos_msg_queue* p = sos_static_ptr()->_msg_queue_ptr;
    if( p )  return p;
       else  return _msg_queue_ptr();
}

//--------------------------------------------------------------------------sos_msg_wait_for_msg_to

void sos_msg_wait_for_msg_to( const Sos_object* object_ptr )
{
    //TYPED_AUTO_PTR( Sos_msg_queue, sos_static_ptr()->_msg_queue_ptr );
    //((Sos_msg_queue*)+(sos_static_ptr()->_msg_queue_ptr))->dispatch_next_msg( 0, object_ptr );
    //auto_new_ptr( &sos_static_ptr()->_msg_queue_ptr )->dispatch_next_msg( 0, object_ptr );
    msg_queue_ptr()->dispatch_next_msg( 0, object_ptr );
}

//-----------------------------------------------------------------Sos_msg_queue::dispatch_next_msg

//uint4 msg_total_count;

int Sos_msg_queue::dispatch_next_msg( int count, const Sos_object* object_ptr, Bool dispatch_os_msg )
{
    int msg_count = 0;

    while( count? msg_count != count : true )
    {
        if( _queue_begin == _queue_end )
        {
            if( !dispatch_os_msg )  return msg_count;       // throw  Xc( "SOS-OBJ-1004" ); ???

            //Increment_semaphore( &sosmsg_dispatcher_active );

            if( Sos_socket::open_event_count() > 0 ) {
                LOGI( "warte auf " << Sos_socket::open_event_count() << " Socket-Botschaften ...\n" );
                //uint4 t = elapsed_msec();
                while(1) {
                    Sos_socket::select();   // Ruft evtl. rekursiv dispatch_waiting_msg() !!!
                    if( _queue_begin != _queue_end )  break;
                }
                //LOG( ( elapsed_msec() - t ) << " ms\n" );
            }
            else
            {
#             if defined SYSTEM_WIN
              //if( windows_open_event_count  gesetzt von e370.mode( m_terminal_input ) ...
                LOGI( "warte auf Windows-Botschaften ...\n" );

                Bool abbruch = false;
              //Wait_msg_box wait_msg_box ( &abbruch, 10, "xx" );

              //uint4 t = elapsed_msec();

                MSG msg;

                while( GetMessage( &msg, 0, 0, 0 ) ) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    if( _queue_begin != _queue_end )  break;
                    if( abbruch ) break;
                }

                //LOG( ( elapsed_msec() - t ) << " ms\n" );

                if( msg.message == WM_QUIT ) {
                    SHOW_ERR( "sosmsg: Die Operation wird abgebrochen" );
                    PostQuitMessage( 1 );
                    throw_abort_error();
                }

                if( abbruch )  throw_abort_error();
#             endif
            }

            if( _queue_begin == _queue_end )  throw_xc( "SOS-OBJ-1004" );
        }

        Sos_msg_queue::Queue_entry* e = &_msg_array[ _queue_begin ];
        Sos_ptr<Sos_msg>  m = e->_msg_ptr;
        e->_msg_ptr = 0;
        const Sos_object* dest_ptr   = m->dest_ptr();

        _queue_begin++;
        if( _queue_begin > _msg_array.last_index() )  _queue_begin = _msg_array.first_index();

        if( m->dest_ptr()->obj_ref_count() < 1 )  throw_xc( "SOS-1151" ); // ^ (Sos_object*)0 ^ *ms_ptr, wenn Sos_object_base

        if( m->is_request() )  m->dest_ptr()->_request_msg( (Request_msg*)+m );
                         else  m->dest_ptr()->_reply_msg  ( (Reply_msg*)+m );

        msg_count++;

        if( dest_ptr == object_ptr )  break;
    }

    return msg_count;
}

//-------------------------------------------------------------------------------sos_msg_dispatcher

void dispatch_waiting_msg()
{
    msg_queue_ptr()->dispatch_next_msg( false );
}

//-------------------------------------------------------------------------------sos_msg_dispatcher

void sos_msg_dispatcher()
{
    msg_queue_ptr()->dispatch_next_msg( true );
}

//-------------------------------------------------------------------------------sos_msg_dispatcher

void sos_msg_dispatcher( int count )
{
    msg_queue_ptr()->dispatch_next_msg( count, 0 );
    //TYPED_AUTO_PTR( Sos_msg_queue, sos_static_ptr()->_msg_queue_ptr );
    //((Sos_msg_queue*)+(sos_static_ptr()->_msg_queue_ptr))->dispatch_next_msg( count );
}

//--------------------------------------------------------------------------------------------reply

void _post( Sos_msg* m )
{
    msg_queue_ptr()->enqueue( m );
    //TYPED_AUTO_PTR( Sos_msg_queue, sos_static_ptr()->_msg_queue_ptr );
    //((Sos_msg_queue*)+(sos_static_ptr()->_msg_queue_ptr))->enqueue( m, false );
}

//--------------------------------------------------------------------------------------------reply

void _post( const Sos_msg& m )
{
#   if 1 //VC2003 defined SYSTEM_SOLARIS || defined SYSTEM_GNU
        if( !sos_static_ptr()->_msg_queue_ptr ) {
            Sos_ptr<Sos_msg_queue> p = SOS_NEW_PTR( Sos_msg_queue );
            sos_static_ptr()->_msg_queue_ptr = +p;
        }

        //if( m.type() == msg_data_reply ) LOG( "_post( Data_reply_msg._data.ref=" << ((int*)(((Data_reply_msg*)&m)->_data.ptr()))[-1] << ")   obj_copy()  " );
        //jz 1.11.98 (egcs) const Sos_ptr<Sos_msg>& p = obj_copy( m );
        const Sos_ptr<Sos_msg>& p = OBJ_COPY( Sos_msg, m );
        //if( m.type() == msg_data_reply ) LOG( "Data_reply_msg._data.ref=" << ((int*)(((Data_reply_msg*)&m)->_data.ptr()))[-1] << "\n" );
        sos_static_ptr()->_msg_queue_ptr->enqueue( p );
#    else
        msg_queue_ptr()->enqueue( obj_copy( m )/*, true*/ );
#   endif
    //TYPED_AUTO_PTR( Sos_msg_queue, sos_static_ptr()->_msg_queue_ptr );
    //((Sos_msg_queue*) +(sos_static_ptr()->_msg_queue_ptr))->enqueue( m.new_copy(), true );
}


void reply_ack_msg( Sos_object* dst, Sos_object* src )
{
    Ack_msg m ( dst, src );
    reply( m );
}

void reply_data_reply_msg( Sos_object* dst, Sos_object* src, const Const_area_handle& area )
{
    Data_reply_msg m ( dst, src, area );
    reply( m );
}

void reply_error_msg( Sos_object* dst, Sos_object* src, const Xc& x )
{
    Error_msg m ( dst, src, x );
    reply( m );
}

void reply_error_msg( Sos_object* dst, Sos_object* src, const char* error_code )
{
    reply_error_msg( dst, src, Xc( error_code ) );
}

} //namespace sos
