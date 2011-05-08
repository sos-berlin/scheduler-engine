//#define MODULE_NAME "sam"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include <precomp.h>
#if 0

#include <sysdep.h>
#include <sosstrng.h>
#include <sos.h>
#include <xception.h>
#include <log.h>
#include <sosobj.h>
#include <sosmsg.h>
#include <sosfiltr.h>
#include <sosfact.h>
#include <sosblock.h>
#include <sam.h>

//--------------------------------------------------------------------------Record_as_sam_descr

struct Record_as_sam_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "record/sam"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Record_as_sam> file = SOS_NEW_PTR( Record_as_sam );
        return +file;
    }
};

const Record_as_sam_descr     _record_as_sam_descr;
extern const Sos_object_descr& record_as_sam_descr = _record_as_sam_descr;

//--------------------------------------------------------------------------Sam_as_record_descr

// Benötigt Sos_read_blocker!!

struct Sam_as_record_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "sam/record"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Sam_as_record> file = SOS_NEW_PTR( Sam_as_record );
        return +file;
    }
};

const Sam_as_record_descr     _sam_as_record_descr;
extern const Sos_object_descr& sam_as_record_descr = _sam_as_record_descr;

//----------------------------------------------------------------Record_as_sam::MSG_DISPATCHER

SOS_BEGIN_MSG_DISPATCHER( Record_as_sam )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( data_reply )
SOS_END_MSG_DISPATCHER

//-----------------------------------------------------------------Record_as_sam::Record_as_sam

Record_as_sam::Record_as_sam( /*Sos_object* client*/ )
:
    _length_bytes_count ( 0 )
{
    //_obj_client_ptr = client;
    //obj_created();
}

//----------------------------------------------------------------Record_as_sam::~Record_as_sam

Record_as_sam::~Record_as_sam()
{
    // Virtueller Vor-Destruktor: obj_destruct() wird von Sos_pointer gerufen (?)

    _obj_output_ptr.del();      // vor ~_obj_reverse_filter
    _obj_reverse_filter.del();  // Verweis auf *_obj_reverse_filter löschen vor ~_obj_reverse_filter
}

//----------------------------------------------------Record_as_sam::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Record_as_sam::_obj_create_reverse_filter( Sos_msg_filter* client )
{
    Sos_msg_filter* c = client? client : SOS_CAST( Sos_msg_filter, _obj_client_ptr );

    Sos_ptr<Sam_as_record> p = SOS_NEW_PTR( Sam_as_record() );
    p->obj_output_ptr( c->obj_reverse_filter() );
    return +p;
}

//---------------------------------------------------------Record_as_sam::_obj_input_block_size

uint Record_as_sam::_obj_input_block_size()
{
    ASSERT_VIRTUAL( _obj_input_block_size );

    uint bs = obj_output_ptr()->obj_input_block_size();
    return bs > 3? bs - 3 : 0;
}

//-------------------------------------------------------------Record_as_sam::_obj_input_buffer

Area Record_as_sam::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    Area a = obj_output_ptr()->obj_input_buffer();
    return Area( a.byte_ptr() + 3, a.length() - 3 );
}

//-----------------------------------------------------------------Record_as_sam::_obj_data_msg

void Record_as_sam::_obj_data_msg( Data_msg* m )
{
    _obj_client_ptr = m->source_ptr();
  //_original_dest_ptr = m->dest_ptr();

    uint    length  = m->data().length();
    Byte*   p       = obj_output_ptr()->obj_input_buffer().byte_ptr();

    p[ 0 ] = (Byte) ( length >> 16 );
    p[ 1 ] = (Byte) ( length >>  8 );
    p[ 2 ] = (Byte)   length;

    if( m->data().byte_ptr() != p + 3 )
    {
        //int EIN_PUFFER_ZUVIEL;
        Dynamic_area buffer ( 3 + m->data().length() );
        memcpy( buffer.ptr(), p, 3 );
        memcpy( buffer.byte_ptr() + 3, m->data().ptr(), m->data().length() );
        buffer.length( 3 + m->data().length() );
        obj_send( buffer );
        //obj_send( Const_area( p, 3 ), m->data() );
    }
    else
    {
        obj_send( Const_area( p, 3 + length ) );
    }
}

//-----------------------------------------------------------Record_as_sam::_obj_data_reply_msg

void Record_as_sam::_obj_data_reply_msg( Data_reply_msg* m )
{
    const Byte* p = m->data().byte_ptr();

    if( _length_bytes_count < 3 )
    {
        _get_buffer.allocate_min( 256 );
        _get_buffer.length( 0 );
        int n = min( 3u, m->data().length() - _length_bytes_count );
        memcpy( _length_bytes + _length_bytes_count, m->data().ptr(), n );
        _length_bytes_count += n;
        p += n;

        uint rest = 3 - _length_bytes_count;
        if( rest ) {
            post_request( Get_msg( m->source_ptr(), this, rest ) );
            return;
        }

        uint4 length =   ( (uint)_length_bytes[0] << 16 )
                       | ( (uint)_length_bytes[1] <<  8 )
                       |         _length_bytes[2];

        if( length < 0 || length > 34000 ) {
            LOG( "Record_as_sam: Falsche Länge empfangen: " << hex << length << dec << "." << endl );
            throw_data_error( "SOS-1124" );
        }

        _get_length = length;
        _get_buffer.allocate_min( _get_length );
    }

    uint n = MIN( _get_length - _get_buffer.length(),
                  m->data().byte_ptr() + m->data().length() - p );
    _get_buffer.append( p, n );

    uint rest = _get_length - _get_buffer.length();
    if( rest ) {
        post_request( Get_msg( m->source_ptr(), this, rest ) );
        return;
    }

    // ??? Fehlt nicht bei den anderen returns vor post_request ein reply(Data_reply_msg) ???
    reply( Data_reply_msg( _obj_client_ptr, this, _get_buffer ) );
    _obj_client_ptr = 0;
    _length_bytes_count = 0;
}

//------------------------------------------------------------------Record_as_sam::_obj_ack_msg

void Record_as_sam::_obj_ack_msg( Ack_msg* )
{
    obj_reply_ack();
}

//----------------------------------------------------------------Sam_as_record::MSG_DISPATCHER
/*
SOS_BEGIN_MSG_DISPATCHER( Sam_as_record )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
SOS_END_MSG_DISPATCHER
*/
//-----------------------------------------------------------------Sam_as_record::Sam_as_record

Sam_as_record::Sam_as_record(  )
:
    Sos_reblocker   ( true /*read_blocker*/ ),
    _collect_length ( true )
{
}

//----------------------------------------------------------------Sam_as_record::~Sam_as_record

Sam_as_record::~Sam_as_record()
{
    _obj_output_ptr.del();      // vor ~_obj_reverse_filter
    _obj_reverse_filter.del();  // Verweis auf *_obj_reverse_filter löschen vor ~_obj_reverse_filter
}

//----------------------------------------------------Sam_as_record::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Sam_as_record::_obj_create_reverse_filter( Sos_msg_filter* client )
{
    Sos_msg_filter* c = client? client : SOS_CAST( Sos_msg_filter, _obj_client_ptr );

    Sos_ptr<Record_as_sam> p = SOS_NEW_PTR( Record_as_sam() );
    p->obj_output_ptr( c->obj_reverse_filter() );
    return +p;
}

//-----------------------------------------------------------------Sam_as_record::_obj_input_buffer
/*
Area Sam_as_record::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    return _collect_length? AREA( _length_bytes ) : obj_output_ptr()->obj_input_buffer();
}
*/
//---------------------------------------------------------Sam_as_record::_obj_input_block_size

uint Sam_as_record::_obj_input_block_size()
{
    ASSERT_VIRTUAL( _obj_input_block_size );

    return _collect_length? 3 : _length;
}

//-----------------------------------------------------------------Sam_as_record::_obj_input_buffer
/*
Area Sam_as_record::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    Area a = obj_output_ptr()->obj_input_buffer();
    return Area( a.byte_ptr() + 3, a.length() - 3 );
}
*/
//--------------------------------------------------------------------------Sam_as_record::_obj_msg
/*
void Sam_as_record::_obj_msg( Sos_msg* m )
{
    m->dest_ptr( obj_output_ptr() );
    m->source_ptr( this );
    send( m );
}
*/
//-------------------------------------------------------------Sam_as_record::_blocked_data_msg

void Sam_as_record::_blocked_data_msg( Data_msg* m )
{
    ASSERT_VIRTUAL( _blocked_data_msg );

  //_obj_client_ptr = m->source_ptr();

    if( _collect_length )
    {
        if( m->data().length() != 3 )  throw_xc( "SOS-1134", this );
        const Byte* p = m->data().byte_ptr();
        _length = ( (uint)p[0] << 16 )  |  ( (uint)p[1] <<  8 )  |  p[2];
        _collect_length = false;
        obj_reply_ack();
    }
    else
    {
        if( m->data().length() != _length )  throw_xc( "SOS-1134", this );

        _collect_length = true;
        _length = 0;
        m->dest_ptr( obj_output_ptr() );
        m->source_ptr( this );
        request( m );
    }
}

//------------------------------------------------------------------Sam_as_record::_obj_ack_msg
/*
void Sam_as_record::_obj_ack_msg( Ack_msg* )
{
    obj_reply_ack();
}
*/

#endif
