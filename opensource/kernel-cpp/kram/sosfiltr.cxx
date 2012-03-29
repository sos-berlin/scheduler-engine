//#include "../kram/optimize.h"
#include "precomp.h"
//#define MODULE_NAME "sosfiltr"
//#define COPYRIGHT   "© SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sos.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfiltr.h"
#include "../kram/log.h"

using namespace std;
namespace sos {

//------------------------------------------------------------------Sos_msg_filter::~Sos_msg_filter

Sos_msg_filter::~Sos_msg_filter()
{
    //delete _obj_read_blocker_ptr;
    // Muß wohl in jeder erbenden Klasse geschehen:
    _obj_output_ptr.del();      // vor ~_obj_reverse_filter
    _obj_reverse_filter.del();  // Verweis auf *_obj_reverse_filter löschen vor ~_obj_reverse_filter
}

//--------------------------------------------------------------Sos_msg_filter::_obj_output_ptr
/*
void Sos_msg_filter::obj_output_ptr( const Sos_ptr<Sos_object>& o )
{
    _obj_output_ptr = o;
}
*/
//-----------------------------------------------------------Sos_msg_filter::obj_reverse_filter

Sos_msg_filter* Sos_msg_filter::obj_reverse_filter( Sos_msg_filter* f )
{
    if( !_obj_reverse_filter )
    {
        _obj_reverse_filter = _obj_create_reverse_filter( f );
/*
        Sos_ptr<Sos_msg_filter> reverse_client = _obj_client_ptr->obj_reverse_filter();
        _obj_reverse_filter = _obj_create_reverse_filter();
        _obj_reverse_filter->obj_output_ptr( +reverse_client );
*/
    }
    return _obj_reverse_filter;
}

//---------------------------------------------------Sos_msg_filter::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Sos_msg_filter::_obj_create_reverse_filter( Sos_msg_filter* )
{
    throw_xc( "SOS-1127", this );
    return 0;
}

//---------------------------------------------------------------------Sos_msg_filter::_obj_msg

void Sos_msg_filter::_obj_msg( Sos_msg* m )
{
    if( m->is_request() )
    {
        if( m->type() == msg_open || !obj_output_ptr() ) {
            Base_class::_obj_msg( m );
        }
        else
        {
            _obj_client_ptr = m->source_ptr();
            m->dest_ptr( obj_output_ptr() );
            m->source_ptr( this );
            request( (Request_msg*) m );
        }
    }
    else
    {
        m->dest_ptr( _obj_client_ptr );
        m->source_ptr( this );
        _obj_client_ptr = 0;
        reply( *(Reply_msg*)m );
    }
}

//---------------------------------------------------------------------Sos_msg_filter::msg_send

void Sos_msg_filter::obj_send( const Const_area& data )
{
    Data_msg m ( obj_output_ptr(), this, data );
    request( &m );
}

//---------------------------------------------------------------------Sos_msg_filter::msg_send

void Sos_msg_filter::obj_send( const Const_area& data, uint4 seek_pos )
{
    Data_msg m ( obj_output_ptr(), this, data, seek_pos );
    request( &m );
}

//---------------------------------------------------------------------Sos_msg_filter::msg_send

void Sos_msg_filter::obj_send( const Const_area& data, const Const_area_handle& key )
{
    Data_msg m ( obj_output_ptr(), this, data, key );
    request( &m );
}

//--------------------------------------------------------------Sos_msg_filter::obj_request_end

void Sos_msg_filter::obj_request_end()
{
    End_msg m ( obj_output_ptr(), this );
    request( &m );
}

//----------------------------------------------------------------Sos_msg_filter::obj_reply_ack

void Sos_msg_filter::obj_reply_ack()
{
    reply_ack_msg( _obj_client_ptr, this );
    _obj_client_ptr = 0;
}

//--------------------------------------------------------------Sos_msg_filter::obj_reply_error

void Sos_msg_filter::obj_reply_error( const Xc& x )
{
    reply_error_msg( _obj_client_ptr, this, x );
    _obj_client_ptr = 0;
}

//----------------------------------------------------------------------Sos_msg_filter::obj_reblock
//#if !defined SYSTEM_GNU   // gcc 2.5.8: address of `this' not available
/*
Bool Sos_msg_filter::obj_reblock( Data_msg* m )
{
  //if( _obj_read_blocker_ptr )  return false;

    if( !_obj_read_blocker )  {
        Sos_ptr<Sos_read_blocker> b = SOS_NEW_PTR( Sos_read_blocker( obj_default_buffer_size ) );
        b->obj_output_ptr( this );
        _obj_read_blocker = +b;
    }

    m->dest_ptr( _obj_read_blocker );
    m->source_ptr( this );
    request( m );

    return true;
}
*/

//-------------------------------------------------------------Sos_msg_filter::_obj_client_name

Const_area_handle Sos_msg_filter::_obj_client_name() const
{
	return Base_class::_obj_client_name();

	//if( !_obj_client_ptr )  throw_xc( "Sos_msg_filter::obj_client_name", "_obj_client_ptr == 0 " );
    //return _obj_client_ptr->obj_client_name();
}


//#endif

} //namespace sos
