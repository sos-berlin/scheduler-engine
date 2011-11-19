#include "precomp.h"
//#define MODULE_NAME "sosdumcl"
//#define COPYRIGHT   "© SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/soslimtx.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfact.h"
#include "../kram/sosdumcl.h"
#include "../kram/log.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------------Sos_dummy_client_mailbox

struct Sos_dummy_client_mailbox     // Kommunikationsbereich für Sos_dummy_client
{
                                Sos_dummy_client_mailbox ( Sos_dummy_client* p );
                               ~Sos_dummy_client_mailbox ();

    Sos_msg_type               _replied_msg_type;
    Const_area_handle          _data;
    Const_area_handle          _key;
    Sos_object_ptr             _object_ref;
  //Sos_limited_text<sos_max_error_code_length> _error_code;
  //Sos_ptr<Xc>                 _xc;
    Xc*                        _xc;

  private:
    Sos_dummy_client*          _dummy_client_ptr;
};

//-------------------------------------------Sos_dummy_client_mailbox::Sos_dummy_client_mailbox

Sos_dummy_client_mailbox::Sos_dummy_client_mailbox( Sos_dummy_client* p )
:
    _dummy_client_ptr ( p ),
    _replied_msg_type ( (Sos_msg_type)0 ),
    _xc ( 0 )
{
    if( p->_mailbox_ptr )  throw_xc( "SOS-OBJ-1005" );   // Rekursion
    p->_mailbox_ptr = this;
}

//------------------------------------------Sos_dummy_client_mailbox::~Sos_dummy_client_mailbox

Sos_dummy_client_mailbox::~Sos_dummy_client_mailbox()
{
    _dummy_client_ptr->_mailbox_ptr = 0;
    delete _xc;
}

//---------------------------------------------------------Sos_dummy_client::send_and_await_msg

void Sos_dummy_client::send_and_await_msg( Request_msg* m )
{
    _mailbox_ptr->_replied_msg_type = (Sos_msg_type)0;

    if( _object_ptr )  m->dest_ptr( _object_ptr );
                 else _object_ptr = m->dest_ptr();

    m->source_ptr( this );

    request( m );
    await_msg();
}

//------------------------------------------------------------------Sos_dummy_client::await_ack

void Sos_dummy_client::await_ack()
{
    await_msg();
    if( _mailbox_ptr->_replied_msg_type != (Sos_msg_type)msg_ack )  throw_xc( "SOS-OBJ-1006" );
}

//------------------------------------------------------------------Sos_dummy_client::await_msg

void Sos_dummy_client::await_msg()
{
    //_mailbox_ptr->_replied_msg_type = (Reply_msg_type)0;

    while( _mailbox_ptr->_replied_msg_type == (Sos_msg_type)0 )
    {
        sos_msg_dispatcher( 1 );
    }

    if( _mailbox_ptr->_replied_msg_type == (Sos_msg_type)msg_error )
    {
        if( strcmp( _mailbox_ptr->_xc->code(), "D310" ) == 0 )  throw_eof_error( "D310" );
        if( strcmp( _mailbox_ptr->_xc->code(), "D311" ) == 0 )  throw_not_found_error( "D311" );
        throw *_mailbox_ptr->_xc;
    }
}

//------------------------------------------------------------Sos_dummy_client::send_and_await_data

Const_area_handle Sos_dummy_client::send_and_await_data( Request_msg* m, Const_area_handle* key_ptr )
{
    Sos_dummy_client_mailbox mailbox ( this );

    send_and_await_msg( m );

    if( mailbox._replied_msg_type == (Sos_msg_type)msg_data_reply ) {
        if( key_ptr )  *key_ptr = mailbox._key;
        return mailbox._data;
    }

    throw_xc( "SOS-OBJ-1006" );
    return Const_area_handle();
}

//---------------------------------------------------------Sos_dummy_client::send_and_await_ack

void Sos_dummy_client::send_and_await_ack( Request_msg* m )
{
    Sos_dummy_client_mailbox mailbox ( this );

    send_and_await_msg( m );
    await_ack();
}

//------------------------------------------------------Sos_dummy_client::send_and_await_object_ref

Sos_object_ptr Sos_dummy_client::send_and_await_object_ref( Request_msg* m )
{
    Sos_dummy_client_mailbox mailbox ( this );

    send_and_await_msg( m );

    if( mailbox._replied_msg_type != (Sos_msg_type)msg_object_ref )  throw_xc( "SOS-OBJ-1006" );

    Sos_object_ptr o = mailbox._object_ref;
    mailbox._object_ref = 0;
    return o;
}

//------------------------------------------------Sos_dummy_client::send_and_await_data_request

Const_area_handle Sos_dummy_client::send_and_await_data_request( Data_msg* m )
{
    Sos_dummy_client_mailbox mailbox ( this );

    send_and_await_msg( m );
    if( mailbox._replied_msg_type != (Sos_msg_type)msg_ack )  throw_xc( "SOS-OBJ-1006" );

    await_msg();
    
    if( mailbox._replied_msg_type != (Sos_msg_type)msg_data )  {
        reply_error_msg( _object_ptr, this, "SOS-OBJ-1006" );
        throw_xc( "SOS-OBJ-1006" );
    }

    reply_ack_msg( _object_ptr, this );
    return mailbox._data;
}

//---------------------------------------------------------------------Sos_dummy_client::create

Sos_object_ptr Sos_dummy_client::create( const Sos_string& name, Sos_object* owner )
{
    Sos_dummy_client_mailbox mailbox ( this );
    Sos_object_ptr ptr = 0;
    ptr = sos_factory_ptr()->request_create( this, name, owner );
    await_ack();
    return ptr;
}

//--------------------------------------------------------------Sos_dummy_client::_obj_data_msg

inline void Sos_dummy_client::_obj_data_msg( Data_msg* m )
{
    if( !_mailbox_ptr )   throw_xc( "SOS-OBJ-1007" );

    _mailbox_ptr->_replied_msg_type = (Sos_msg_type)m->type();
    _mailbox_ptr->_data             = m->data();
}

//-------------------------------------------------------------------Sos_dummy_client::_obj_msg

inline void Sos_dummy_client::_obj_other_msg( Sos_msg* m )
{
    if( m->is_reply() ) {
        if( !_mailbox_ptr )   throw_xc( "SOS-OBJ-1007" );
        _mailbox_ptr->_replied_msg_type = m->type();
    } else {
        Base_class::_obj_msg( m );
    }
}

//--------------------------------------------------------------Sos_dummy_client::_obj_data_msg

inline void Sos_dummy_client::_obj_data_reply_msg( Data_reply_msg* m )
{
    if( !_mailbox_ptr )   throw_xc( "SOS-OBJ-1007" );

    _mailbox_ptr->_replied_msg_type = (Sos_msg_type)m->type();
    _mailbox_ptr->_data             = m->data();
  //_mailbox_ptr->_key              = m->key();
}

//--------------------------------------------------------Sos_dummy_client::_obj_object_ref_msg

inline void Sos_dummy_client::_obj_object_ref_msg( Object_ref_msg* m )
{
    if( !_mailbox_ptr )   throw_xc( "SOS-OBJ-1007" );

    _mailbox_ptr->_replied_msg_type = (Sos_msg_type)m->type();
    _mailbox_ptr->_object_ref       = m->object_ptr();
}

//-------------------------------------------------------------Sos_dummy_client::_obj_error_msg

/*inline*/ void Sos_dummy_client::_obj_error_msg( Error_msg* m )
{
    if( !_mailbox_ptr )   throw_xc( "SOS-OBJ-1007" );

    _mailbox_ptr->_replied_msg_type = (Sos_msg_type)m->type();
    //_mailbox_ptr->_error_code = m->error().code();

    //Sos_pointer x = obj_copy( m->error() );
    //_mailbox_ptr->_xc = (Xc*)+x;
    _mailbox_ptr->_xc = new Xc( m->error() );
}

//---------------------------------------------------------------------Sos_dummy_client::_obj_print

void Sos_dummy_client::_obj_print( ostream* s ) const
{
//VC2003    ASSERT_VIRTUAL( _obj_print );

    *s << "Sos_dummy_client";
}

//---------------------------------------------------Sos_dummy_client::SOS_BEGIN_MSG_DISPATCHER

SOS_BEGIN_MSG_DISPATCHER( Sos_dummy_client )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( data_reply  )
    SOS_DISPATCH_MSG( object_ref )
    SOS_DISPATCH_MSG( error )
SOS_END_MSG_DISPATCHER_DEFAULT( _obj_other_msg )


} //namespace sos
