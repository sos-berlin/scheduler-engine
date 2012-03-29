#include "precomp.h"
//#define MODULE_NAME "sosobj"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#define DONT_LOG

#include <limits.h>

#include "../kram/sysdep.h"
#include "../kram/sysxcept.h"

#if defined SYSTEM_RTTI
//? 4.3.03 #   include <typeinfo.h>
#endif

#if defined __BORLANDC__
#   include "../kram/borstrng.h"    // Für xmsg
#endif

#include "../kram/sosstrng.h"       // <- storable.h <- soslimtx.h <- sosobjd.h

#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../kram/msec.h"
#include "../kram/stdfield.h"       // für Sos_object::_obj_print_spec_msg
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
//#include "../kram/sosobjd.h"        // Sos_object_register::unregister_object()
#include "../kram/sosfact.h"
#include "../kram/sosdumcl.h"

using namespace std;
namespace sos {

Bool Sos_object::_busy = false;

//-----------------------------------------------------------------------------throw_busy_error

void throw_busy_error( const Sos_object* o, const char* e )
{
    throw Busy_error( o, e );
}

//----------------------------------------------------------------------Sos_object::~Sos_object
/*  Jetzt in ~Sos_server_proxy. Wieder aktivieren, wenn allgemeines Objektregister eingeführt wird!
Sos_object::~Sos_object()
{
    if( _obj_index ) {
        LOG( this << "->~Sos_object(): Objekt wird aus dem Register entfernt\n" );
        _obj_ref_count++;
        _object_register_ptr->unregister_object( this ); // ruft evtl. ~Sos_object() rekursiv!
        _obj_ref_count--;
    }
}
*/
//----------------------------------------------------------------------------Sos_object::close

void Sos_object::close( Close_mode )
{
}

//-----------------------------------------------------------------------------Sos_obj::obj_run

void Sos_object::obj_run()
{
    Sos_dummy_client dummy_client;
    Run_msg          m             ( this, &dummy_client );

    dummy_client.send_and_await_ack( &m );
}

//-----------------------------------------------------------------------------Sos_obj::obj_end

void Sos_object::obj_end( Close_mode )
{
    Sos_dummy_client dummy_client;
    End_msg          m             ( this, &dummy_client );

    dummy_client.send_and_await_ack( &m );
}

//---------------------------------------------------------------------------------Sos_obj::obj_put

void Sos_object::obj_put( const Const_area& data )
{
    Sos_dummy_client dummy_client;
    Data_msg         m             ( this, &dummy_client, data );

    dummy_client.send_and_await_ack( &m );
}

//---------------------------------------------------------------Sos_obj::obj_put_and_wait_data

Const_area_handle Sos_object::obj_put_and_wait_data( const Const_area& data )
{
    Sos_dummy_client dummy_client;
    Data_msg         m             ( this, &dummy_client, data );

    return dummy_client.send_and_await_data_request( &m );
}

//-------------------------------------------------------------------------Sos_object::obj_read

Const_area_handle Sos_object::obj_read( uint length )
{
    Sos_dummy_client dummy_client;
    Get_msg          m             ( this, &dummy_client, length );

    return dummy_client.send_and_await_data( &m );
}

//--------------------------------------------------------------------------Sos_object::obj_get

Const_area_handle Sos_object::obj_get( Const_area_handle* position_ptr )
{
    Sos_dummy_client dummy_client;
    Get_msg          m             ( this, &dummy_client );

    return dummy_client.send_and_await_data( &m, position_ptr );
}

//-------------------------------------------------------------------------Sos_object::obj_busy

void Sos_object::obj_busy()
{
    _busy = true;
    //throw Busy_error();
}

//----------------------------------------------------------------------Sos_object::obj_reply_error
/*
void Sos_object::obj_reply_error( const Xc& x ) const
{

}
*/
//----------------------------------------------------------------------Sos_object::_obj_possession
/*
Sos_object* Sos_object::_obj_possession( int ) const
{
    return 0;
}
*/
//-------------------------------------------------------------------------Sos_object::_obj_msg

void Sos_object::_obj_msg( Sos_msg* m )
{
    switch( m->type() )
    {
        case msg_end  :
        {
            reply_ack_msg( m->source_ptr(), this );
            break;
        }

        case msg_destroy:
        {                                       // Keine Bestätigung möglich
            LOGI( "close " << *this << '\n' );
            obj_remove_ref();
          //int OBJ_REMOVE_REF_DIREKT_GERUFEN;
            return;
        }

        case msg_open:
        {
            if( !empty( ((Open_msg*)m)->name() ) )  throw_xc( "SOS-1135", Msg_insertions( this, ((Open_msg*)m)->name() ) );
            reply_ack_msg( m->source_ptr(), this );
            break;
        }

        case msg_ack:
        case msg_cancel: LOG( *this << '.' << *m << " ignoriert\n" ); break;

        default:
        {
            //LOG( *this << ": Nicht behandelte Botschaft " << *m << '\n' );
            throw_xc( "SOS-1137", Msg_insertions( this , m ) );
        }
    }
}

//----------------------------------------------------------------Sos_object::_obj_input_block_size

uint Sos_object::_obj_input_block_size()
{
    return UINT_MAX;
}

//--------------------------------------------------------------------Sos_object::_obj_input_buffer

Area Sos_object::_obj_input_buffer()
{
    _obj_default_input_buffer.allocate_min( obj_default_buffer_size );
    return _obj_default_input_buffer;
}

//---------------------------------------------------------------------------Sos_object::_obj_print
#if !defined SYSTEM_RTTI

void Sos_object::_obj_print( ostream* s ) const
{
    ASSERT_VIRTUAL( _obj_print );

    *s << "Sos_object";
}

#endif
//----------------------------------------------------------------Sos_object::_obj_print_status

void Sos_object::_obj_print_status( ostream* s ) const
{
    _obj_print( s );
}

//------------------------------------------------------------------Sos_object::_obj_print_spec_msg

void Sos_object::_obj_print_spec_msg( ostream* s, const Spec_msg& msg ) const
{
    const Record_type* f = obj_spec_msg_field_type( msg.spec_code() );

  //*s << "Spec_msg( " << *msg.dest_ptr() << ", " << *msg.source_ptr() << ", ";
  //*s << '[' << *msg.source_ptr() << "] " << *msg.dest_ptr() << "._obj_spec_msg( ";
    *s << "Spec_msg( " << msg.spec_code() << ", len=" << msg.data().length() << " )";

    if( f ) {
        f->print( msg.data().byte_ptr(), s, Text_format() );
    } else {
        *s << msg.spec_code();
    }

    *s << " )";
}

//------------------------------------------------------------------------Sos_object::obj_owner

Sos_object* Sos_object::obj_owner() const
{
    //LOG( *this << ".obj_owner() liefert " << *_owner << endl );
    return _owner;
}

//------------------------------------------------------------------------Sos_object::obj_owner

void Sos_object::obj_owner( Sos_object* o )
{
    //LOG( *this << ".obj_owner( " << *o << " )\n" );
    _owner = o;
}

//---------------------------------------------------------------------Sos_obj::obj_client_name

Const_area_handle Sos_object::obj_client_name() const
{
    return _obj_client_name();
}

//--------------------------------------------------------------------Sos_obj::_obj_client_name

Const_area_handle Sos_object::_obj_client_name() const
{
    //LOGI( *this << "._obj_client_name()  _owner=" << *_owner << "\n" );
    if( !_owner )  throw_xc( "SOS-1178", this );
    return _owner->obj_client_name();
}

//-------------------------------------------------------------------------Sos_obj::_obj_client

Sos_client* Sos_object::_obj_client()
{
    //LOGI( *this << "._obj_client()  _owner=" << *_owner << "\n" );
    if( _owner ) {
        return _owner->obj_client();
    } else {
        Sos_static* sos_static = sos_static_ptr();
        if( sos_static->_multiple_clients )   throw_xc( "SOS-1244", this );
        if( !sos_static->_std_client )  throw_xc( "SOS-1244", "**sos_static**" );
        return sos_static->_std_client;
    }
}

//------------------------------------------------------------------------Sos_obj::_request_msg

void Sos_object::_request_msg( Request_msg* m )
{
    Sos_object* source_ptr = m->source_ptr();

    if( _busy )                throw_xc( "SOS-1136", Msg_insertions( source_ptr, m ) );
    if( !this )                throw_xc( "SOS-1131", Msg_insertions( source_ptr, m ) );  // Gibt's mich überhaupt?
    if( obj_ref_count() < 1 )  throw_xc( "SOS-1151", Msg_insertions( source_ptr, m ) );

    Bool  was_busy = false;
    Big_int busy_start;

    try
    {
        while(1)
        {
            try
            {
                //LOGI( *source_ptr << " --> " << *this << '.' << *m << '\n' );

                if( m->type() == msg_cancel )
                {
                    _obj_msg( m );
                }
                else
                {
                    _obj_request_semaphore--;              // s.a. reply()

                    if( m->type() == msg_destroy ) {    // _obj_request_semaphore und m->source_ptr() sind egal
                        _obj_msg( m );
                    }
                    else
                    {
                        if( !source_ptr )  throw_xc( "SOS-1133", Msg_insertions( this, m  ) );

                        if( _obj_request_semaphore >= 0 )
                        {
                            _obj_msg( m );

                            //if( exception() )  throw Xc( _XC.name() );
                            if( !_busy )  break;
                            LOG( *this << " ist beschäftigt\n" );
                        }
                        else
                        {
                            LOG_ERR( *this << "._obj_request_semaphore == " << (_obj_request_semaphore+1) << '\n' );
                            _busy = true;
                        }
                    }
                }
            }
            catch( const Busy_error& x )
            {
                if( !was_busy ) {
                    was_busy   = true;
                    busy_start = elapsed_msec();
                }

                LOG( *this << ": " << x << '\n' );
                _busy = true;
            }

            if( _busy ) {
                _obj_request_semaphore++;
                _busy = false;
                do {
                    sos_msg_dispatcher( 1 );
                } 
                while( !_obj_request_semaphore );
            }
        }
    }

    catch( const Xc& x )
    {
        if( source_ptr ) {
            //LOGI( "reply( Error_msg( " << *source_ptr << ", " << *this << ", " << x << ") );\n");
            reply_error_msg( source_ptr, this, x );
        } else {
            LOG_ERR( x << ", wird mangels Absender unterdrückt\n" );
        }
    }

    catch( const exception& x )
    {
        //LOGI( "request_msg(): exception " << name( x ) << "(\"" << text( x ) << "\") catched\n" );
        if( source_ptr ) {
            reply_error_msg( source_ptr, this, Xc( exception_name( x ).c_str(), exception_text( x ) ) );
        }
    }

    catch( ... )
    {
        LOGI( "request_msg(): unknown exception catched\n" );
        if( source_ptr ) {
#           if defined __BORLANDC__
                reply_error_msg( source_ptr, this, Xc( __throwExceptionName ) );
#            else
                reply_error_msg( source_ptr, this, Xc( "UNKNOWN" /*exception_name()*/ ) );
#           endif
        }
    }

    if( was_busy ) {
        LOG( "Botschaft zugestellt nach " << uint4( elapsed_msec() - busy_start ) << "ms\n" );
    }
}

//--------------------------------------------------------------------------Sos_obj::_reply_msg

void Sos_object::_reply_msg( Reply_msg* m )
{
    //LOGI( *this << '.' << *m << " <-- " << *m->source_ptr() << '\n' );

    if( !this )  {
        throw_xc( "SOS-1132", Msg_insertions( m, m->source_ptr() ) );
    }
    _obj_msg( m );
}

//--------------------------------------------------------------------------Sos_obj::obj_create
/*
Sos_object_ptr Sos_object::obj_create( const char* name )
{
    Sos_dummy_client dummy_client;
    Create_msg       m ( this, &dummy_client, name );
    return dummy_client.send_and_await_object_ref( &m );
}

//--------------------------------------------------------------------------Sos_obj::obj_create

Sos_object_ptr Sos_object::obj_create( const char* name, const File_spec& file_spec )
{
    Sos_dummy_client dummy_client;
    Create_msg       m ( this, &dummy_client, name );
    return dummy_client.send_and_await_object_ref( &m );
}
*/
//-------------------------------------------------------------------Sos_object::obj_output_ptr
/*
void Sos_object::obj_output_ptr( const Sos_ptr<Sos_object>& )
{
    throw_xc( "SOS-1125", *this );
}

//---------------------------------------------------------------Sos_object::obj_reverse_filter

Sos_msg_filter* Sos_object::obj_reverse_filter()
{
    throw_xc( "SOS-1128", *this );
}
*/
//---------------------------------------------------------------Sos_object::obj_sos_msg_filter
/*
Sos_msg_filter& Sos_object::obj_sos_msg_filter()
{
    throw_xc( "SOS-1125" );
    #if defined SYSTEM_SOLARIS
        return 0;
    #endif
}
*/
} //namespace sos
