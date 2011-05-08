//#define MODULE_NAME "nl"
//#define COPYRIGHT   "(c) 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "string.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../kram/sosfiltr.h"
#include "../kram/sosfact.h"

namespace sos {
using namespace std;


//---------------------------------------------------------------------------------Record_as_nl

struct Record_as_nl : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )

                                Record_as_nl            ()  : _send_nl ( false ), _get_seek(0), _get_ptr(0) {}

  protected:
    void                       _obj_msg                 ( Sos_msg* );
#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Record_as_nl"; }
#   endif

  private:
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_get_msg             ( Get_msg* );
    void                       _obj_data_reply_msg      ( Data_reply_msg* );

    Dynamic_area               _get_block;
    long                       _get_seek;
    char*                      _get_ptr;
    Dynamic_area               _get_record;

    Bool                       _send_nl;
};

//---------------------------------------------------------------------------Record_as_nl_descr

struct Record_as_nl_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "record_as_nl"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Record_as_nl> file = SOS_NEW_PTR( Record_as_nl );
        return +file;
    }
};

       const Record_as_nl_descr _record_as_nl_descr;
extern const Sos_object_descr&   record_as_nl_descr = _record_as_nl_descr;

//---------------------------------------------------------------------------------Nl_as_record

struct Nl_as_record : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )

                              //Nl_as_record            ()   {}

  protected:
    void                       _obj_msg                 ( Sos_msg* );
#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Nl_as_record"; }
#   endif

  private:
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _process_input           ( const Byte*, uint );

    Dynamic_area               _buffer;
};

//---------------------------------------------------------------------------Nl_as_record_descr

struct Nl_as_record_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "nl_as_record"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Nl_as_record> file = SOS_NEW_PTR( Nl_as_record );
        return +file;
    }
};

       const Nl_as_record_descr _nl_as_record_descr;
extern const Sos_object_descr&   nl_as_record_descr = _nl_as_record_descr;

//-------------------------------------------------------------------Record_as_nl::_obj_data_msg

inline void Record_as_nl::_obj_data_msg( Data_msg* m )     // auch store, insert etc. !!!!!!!!
{
    _obj_client_ptr = m->source_ptr();

    m->dest_ptr( obj_output_ptr() );
    m->source_ptr( this );
    request( m );

    _send_nl = true;
}

//-------------------------------------------------------------------Record_as_nl::_obj_ack_msg

inline void Record_as_nl::_obj_ack_msg( Ack_msg* )
{
    if( _send_nl )
    {
        _send_nl = false;
#       if defined NL_IS_CRLF
            Data_msg d ( obj_output_ptr(), this, Const_area( "\r\n" ) );
#        else
            Data_msg d ( obj_output_ptr(), this, Const_area( "\n" ) );
#       endif
        request( &d );
    }
    else
    {
        obj_reply_ack();
    }
}

//-------------------------------------------------------------------Record_as_nl::_obj_get_msg

/*inline*/ void Record_as_nl::_obj_get_msg( Get_msg* m )
{
    _obj_client_ptr = m->source_ptr();

    const char* nl       = 0;
    uint        rest_len = 0;

    if( _get_ptr ) {
        rest_len = _get_block.char_ptr() + _get_block.length() - _get_ptr;
        nl = (const char*)memchr( _get_ptr, '\n', rest_len );
    }

    if( nl ) {
        uint len = nl - _get_ptr;
        _get_record.assign( _get_ptr, len > 0  &&  _get_block.char_ptr()[ len-1 ] == '\r'? len - 1 : len );
        reply( Data_reply_msg( _obj_client_ptr, this, _get_record, _get_seek ) );
        _obj_client_ptr = 0;
        len++;
        _get_ptr += len;
    }
    else
    {
        if( rest_len ) {
            memmove( _get_block.char_ptr(), _get_ptr, rest_len );
            _get_block.length( rest_len );
            _get_ptr = _get_block.char_ptr();    // vorsichtshalber
        } else {
            _get_block.length( 0 );
        }

        m->dest_ptr( obj_output_ptr() );
        m->source_ptr( this );
        request( m );
    }
}

//------------------------------------------------------------Record_as_nl::_obj_data_reply_msg

/*inline*/ void Record_as_nl::_obj_data_reply_msg( Data_reply_msg* m )
{
    if( _get_block.length() == 0 )  _get_seek = m->seek_pos();

    const char* nl = (const char*)memchr( m->data().ptr(), '\n', m->data().length() );
    if( nl ) {
        uint len = nl - m->data().char_ptr();
        _get_block.append( m->data().char_ptr(), len );
        if( _get_block.length() > 0  &&  _get_block.char_ptr()[ _get_block.length()-1 ] == '\r' ) {
            _get_block.length( _get_block.length() - 1 );
        }

        reply( Data_reply_msg( _obj_client_ptr, this, _get_block, _get_seek  ) );
        _obj_client_ptr = 0;

        len++;
        _get_block.assign( m->data().char_ptr() + len, m->data().length() - len );
        _get_seek = (long)m->seek_pos() == -1? -1 : (long)m->seek_pos() + (long)len;
    }
    else
    {
        _get_block.append( m->data() );
        Get_msg get_msg ( obj_output_ptr(), this );
        request( &get_msg );
    }

    _get_ptr = _get_block.char_ptr();
}

//-----------------------------------------------------------------------Record_as_nl::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Record_as_nl )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack )
    SOS_DISPATCH_MSG( get )
    SOS_DISPATCH_MSG( data_reply )
SOS_END_MSG_DISPATCHER

//-------------------------------------------------------------------Nl_as_record::_obj_data_msg

void Nl_as_record::_obj_data_msg( Data_msg* m )     // auch store, insert etc.!!!!
{
    _process_input( m->data().byte_ptr(), m->data().length() );
}

//-----------------------------------------------------------------Nl_as_record::_process_input

void Nl_as_record::_process_input( const Byte* ptr, uint len )
{
    const Byte* nl_ptr = (const Byte*)memchr( _buffer.ptr(), '\n', _buffer.length() );

    if( nl_ptr )
    {
        obj_send( Const_area( _buffer.ptr(), nl_ptr - _buffer.byte_ptr() ) );

        uint l = nl_ptr + 1 - _buffer.byte_ptr();
        memmove( _buffer.ptr(), _buffer.byte_ptr() + l, _buffer.length() - l );
        _buffer.length( _buffer.length() - l );
    }
    else
    {
        nl_ptr = (const Byte*)memchr( ptr, '\n', len );
        uint l = nl_ptr? nl_ptr - ptr : len;

        if( nl_ptr && _buffer.length() == 0 )
        {
            obj_send( Const_area( ptr, l ) );
            l++;
            len -= l;
            ptr += l;
        }
        else
        {
            _buffer.append( ptr, l );
            len -= l;
            ptr += l;

            if( nl_ptr ) {
                obj_send( _buffer );
                len--; ptr++;
            }
            else
            {
                obj_reply_ack();
            }
        }
    }

    _buffer.append( ptr, len );
}

//-------------------------------------------------------------------Nl_as_record::_obj_ack_msg

void Nl_as_record::_obj_ack_msg( Ack_msg* )
{
    _process_input( 0, 0 );
}

//-----------------------------------------------------------------------Nl_as_record::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Nl_as_record )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack )
SOS_END_MSG_DISPATCHER

} //namespace sos
