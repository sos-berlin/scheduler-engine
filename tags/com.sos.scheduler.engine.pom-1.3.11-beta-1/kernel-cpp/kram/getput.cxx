#include "precomp.h"
#define MODULE_NAME "getput"
#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfiltr.h"
#include "../kram/sosfact.h"

//--------------------------------------------------------------------------Get_as_put_receiver

struct Get_as_put_receiver : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )

                                Get_as_put_receiver     ( struct Get_as_put* g ) :  _get_as_put(g),_record_valid ( false ), _eof(false),_run_msg_sent(false) {}


  protected:
    SOS_DECLARE_MSG_DISPATCHER

  private:
    friend class                Get_as_put;

    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    Bool                        record_valid            ();

    struct Get_as_put*         _get_as_put;
    Const_area_handle          _record;
    Const_area_handle          _key;
    Bool                       _record_valid;
    Bool                       _run_msg_sent;
    Bool                       _eof;
};

//-----------------------------------------------------------------------------------Get_as_put

struct Get_as_put : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )
                                Get_as_put              ()         : _receiver(this) {}
                               ~Get_as_put              ();

  protected:
    uint                       _obj_input_block_size    ();
    Area                       _obj_input_buffer        ();
    Sos_ptr<Sos_msg_filter>    _obj_create_reverse_filter( Sos_msg_filter* );
    SOS_DECLARE_MSG_DISPATCHER

  private:
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_get_msg             ( Get_msg* );

    Get_as_put_receiver        _receiver;
};

//-----------------------------------------------------------------------------Get_as_put_descr

struct Get_as_put_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "get_as_put"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Get_as_put> file = SOS_NEW_PTR( Get_as_put );
        return +file;
    }
};

const Get_as_put_descr         _get_as_put_descr;
extern const Sos_object_descr&  get_as_put_descr = _get_as_put_descr;

//-----------------------------------------------------------Get_as_put_receiver::_obj_data_msg

inline void Get_as_put_receiver::_obj_data_msg( Data_msg* m )
{
    if( _record_valid )  { obj_busy(); return; }

    _record = m->data();
    _key    = m->key();
    _record_valid = true;

    reply( Ack_msg( m->source_ptr(), this ) );
}

//------------------------------------------------------------Get_as_put_receiver::_obj_ack_msg

inline void Get_as_put_receiver::_obj_ack_msg( Ack_msg* m )
{
    // Antwort auf Run_msg()
    _eof = true;
}

//---------------------------------------------------------------------Get_as_put::_obj_get_msg

/*inline*/ Bool Get_as_put_receiver::record_valid()
{
    if( !_run_msg_sent ) {
        _run_msg_sent = true;
        Run_msg m ( //SOS_CAST( Sos_msg_filter,
                              _get_as_put->obj_output_ptr() /*)->obj_reverse_filter()*/,
                    this );
        request( &m );
    }

    return _record_valid;
}

//----------------------------------------------------------------Get_as_put_receiver::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Get_as_put_receiver )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
SOS_END_MSG_DISPATCHER

//-----------------------------------------------------------------------Get_as_put::Get_as_put

Get_as_put::~Get_as_put()
{
    _obj_output_ptr.del();      // vor ~_receiver!
    _obj_reverse_filter.del();  // Verweis auf _receiver löschen vor ~_receiver
}

//------------------------------------------------------------Get_as_put::_obj_input_block_size

uint Get_as_put::_obj_input_block_size()
{
    ASSERT_VIRTUAL( _obj_input_block_size );

    return obj_output_ptr()->obj_input_block_size();
}

//----------------------------------------------------------------Get_as_put::_obj_input_buffer

Area Get_as_put::_obj_input_buffer()
{
    ASSERT_VIRTUAL( _obj_input_buffer );

    return obj_output_ptr()->obj_input_buffer();
}

//-------------------------------------------------------Get_as_put::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Get_as_put::_obj_create_reverse_filter( Sos_msg_filter* )
{
    ASSERT_VIRTUAL( _obj_create_reverse_filter )

    return &_receiver;
}

//--------------------------------------------------------------------Get_as_put::_obj_data_msg

inline void Get_as_put::_obj_data_msg( Data_msg* m )
{
    if( _receiver._eof ) { _receiver._eof = false; LOG( *this << "._eof=false\n" ); }   // Für Fileserver, welcher Run_msg sofort bestätigt (denn wann sonst?)

    _obj_client_ptr = m->source_ptr();
    m->source_ptr( this );
    m->dest_ptr( obj_output_ptr() );
    request( m );
}

//---------------------------------------------------------------------Get_as_put::_obj_ack_msg

inline void Get_as_put::_obj_ack_msg( Ack_msg* )
{
    obj_reply_ack();
}

//---------------------------------------------------------------------Get_as_put::_obj_get_msg

inline void Get_as_put::_obj_get_msg( Get_msg* m )
{
    if( _receiver._eof )  throw_eof_error();
/*
    if( !_run_msg_sent ) {
        _run_msg_sent = true;
        Run_msg m ( _get_as_put->obj_output_ptr(), this );
        request( &m );
    }

*/
    //if( _receiver.record_valid() ) {
    if( _receiver._record_valid ) {
        _receiver._record_valid = false;
        reply( Data_reply_msg( m->source_ptr(), this, _receiver._record, _receiver._key ) );
    }
    else obj_busy();
}

//-------------------------------------------------------------------------Get_as_put::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Get_as_put )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( get  )
SOS_END_MSG_DISPATCHER



