#ifndef __GNUC__

#include <optimize.h>
#include <precomp.h>
#define MODULE_NAME "sosblock"
#define COPYRIGHT   "(c) SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <stdlib.h>
#include <sosstrng.h>
#include <sos.h>
#include <limits.h>
#include <xception.h>
#include <log.h>
#include <sosobj.h>
#include <sosmsg.h>
#include <sosfiltr.h>
#include <sosfact.h>
#include <sosblock.h>

//-----------------------------------------------------------------------Sos_read_blocker_descr
/*
struct Sos_read_blocker_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "read_blocker"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Sos_read_blocker> file = SOS_NEW_PTR( Sos_read_blocker() );
        return +file;
    }
};

const Sos_read_blocker_descr  _sos_read_blocker_descr;
extern const Sos_object_descr& sos_read_blocker_descr = _sos_read_blocker_descr;

//----------------------------------------------------------------------Sos_write_blocker_descr

struct Sos_write_blocker_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "write_blocker"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Sos_write_blocker> file = SOS_NEW_PTR( Sos_write_blocker() );
        return +file;
    }
};

const Sos_write_blocker_descr  _sos_write_blocker_descr;
extern const Sos_object_descr&  sos_write_blocker_descr = _sos_write_blocker_descr;
*/
//----------------------------------------------------------------------Sos_reblocker::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Sos_reblocker )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( open )
SOS_END_MSG_DISPATCHER

//-----------------------------------------------------------------Sos_reblocker::Sos_reblocker

Sos_reblocker::Sos_reblocker( Bool block_read )
:
    //_input_block_size  ( 2048 ),
    _output_block_size ( 2048 ),
    _block_read        ( block_read ),
    _ack               ( false ),
    _in_process_input  ( 0 )
{
}

//-----------------------------------------------------------------Sos_reblocker::Sos_reblocker

void Sos_reblocker::_obj_open_msg( Create_msg* m )
{
    const char* p = c_str( m->name() );

    if( memcmp( p, "-size", 5 ) == 0 ) {
        p += 5;
        int4 size = atoi( p );
        if( !size )  throw_xc( "Sos_reblocker-size" );
        //_input_block_size = size;
        _output_block_size = size;
    }

    _obj_client_ptr = m->source_ptr();
    obj_created();
}

//---------------------------------------------------------Sos_reblocker::_obj_input_block_size
/*
uint Sos_reblocker::_obj_input_block_size()
{
    ASSERT_VIRTUAL( _obj_input_block_size )

    return _input_block_size;
}
*/
//----------------------------------------------------Sos_reblocker::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Sos_reblocker::_obj_create_reverse_filter( Sos_msg_filter* )
{
    ASSERT_VIRTUAL( _obj_create_reverse_filter )

    return SOS_CAST( Sos_msg_filter, _obj_client_ptr )->obj_reverse_filter( this );
}

//-----------------------------------------------------------------Sos_reblocker::_obj_data_msg

void Sos_reblocker::_obj_data_msg( Data_msg* m )
{
    _obj_client_ptr = m->source_ptr();
    _input_buffer.assign( m->data() );
    _input_ptr  = _input_buffer.byte_ptr();
    _input_end  = _input_ptr + _input_buffer.length();
    _process_input();
}

//-----------------------------------------------------------------Sos_reblocker::_obj_ack_msg

void Sos_reblocker::_obj_ack_msg( Ack_msg* )
{
/* nach send():
    if( _block_read ) {
        _buffer.length( 0 );
    } else {
        if( _buffer.length() ) {
            uint len = _buffer.length() - _output_block_size;
            memmove( _buffer.ptr(), _buffer.byte_ptr() + _buffer.length(), len );
            _buffer.length( len );
        }
    }
*/
/*
    if( _eof ) {
        obj_send_end();
        return;
    }
*/
    obj_reply_ack();
    _ack = false;     // jz 12.11.95
}

//-----------------------------------------------------------------Sos_reblocker::obj_reply_ack

void Sos_reblocker::obj_reply_ack()
{
    assert( !_ack );
    _ack = true;

    _buffer.length( 0 );

    if( !_in_process_input ) {
        _process_input();
    }
}

//------------------------------------------------------------------Sos_reblocker::_obj_end_msg
/*
void Sos_reblocker::_obj_end_msg( End_msg* )
{
    ASSERT_VIRTUAL( _obj_end_msg );

    if( _buffer.length() ) {
        obj_send( _buffer );
        _eof = true;
    } else {
        obj_send_end();
    }
}
*/
//----------------------------------------------------------------Sos_reblocker::_process_input

void Sos_reblocker::_process_input()
{
    Increment_semaphore<int> x_ ( &_in_process_input );

    if( _buffer.length() == 0 )     // kein Rest vom letzen Mal
    {
        while(1) {
            _output_block_size = obj_input_block_size();
            if( _input_end - _input_ptr < _output_block_size )  break;
            _input_ptr += _output_block_size;
            send_data( Const_area( _input_ptr - _output_block_size, _output_block_size ) );
            if( !_ack )  break;
        }

        //if( _input_ptr == _input_end )
        {
            if( _ack ) {
                _ack = false;
                reply( Ack_msg( _obj_client_ptr, this ) );
                _obj_client_ptr = 0;
            }
            return;
        }

        _buffer = obj_input_buffer();
    }

    uint len = min( _buffer.size() - _buffer.length(), (uint)( _input_end - _input_ptr ));

    _buffer.append( _input_ptr, len );
    _input_ptr += len;

    if( _buffer.length() < _buffer.size() ) {
        _ack = true;
    } else {
        send_data( _buffer );
    }

    if( _ack ) {
        _ack = false;
        reply( Ack_msg( _obj_client_ptr, this ) );
        _obj_client_ptr = 0;
    }
}

//---------------------------------------------------------------------Sos_reblocker::send_data

void Sos_reblocker::send_data( const Const_area& record )
{
    _ack = false;   //?

    Data_msg data_msg( this, _obj_client_ptr, record );

    _blocked_data_msg( &data_msg );
}

//----------------------------------------------------------Sos_read_blocker::_obj_input_buffer
/*
Area Sos_read_blocker::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    _obj_default_input_buffer.allocate_min( _input_block_size );
    return _obj_default_input_buffer;
}

//----------------------------------------------------------Sos_read_blocker::_blocked_data_msg

void Sos_read_blocker::_blocked_data_msg( Data_msg* m )
{
    m->dest_ptr( obj_output_ptr() );
    m->source_ptr( this );
    request( m );
}

//---------------------------------------------------------Sos_write_blocker::Sos_write_blocker

Sos_write_blocker::Sos_write_blocker()
:
    Sos_reblocker( false )
{
    if( (unsigned long)_output_block_size + obj_default_buffer_size > (unsigned long)UINT_MAX )
    {
        throw_xc( "Sos_write_blocker-size" );
    }

  exceptions
}

//---------------------------------------------------------Sos_write_blocker::_obj_input_buffer

Area Sos_write_blocker::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    if( _buffer.size() == 0 ) {
        _output_buffer.allocate_min( _output_block_size + obj_default_buffer_size );
        _buffer = _output_buffer;
    }

    return Area( _buffer.byte_ptr() + _buffer.length(), _buffer.size() - _buffer.length() );
}

//---------------------------------------------------------Sos_write_blocker::_blocked_data_msg

void Sos_write_blocker::_blocked_data_msg( Data_msg* m )
{
    m->dest_ptr( obj_output_ptr() );
    m->source_ptr( this );
    request( m );
}
*/


#endif
