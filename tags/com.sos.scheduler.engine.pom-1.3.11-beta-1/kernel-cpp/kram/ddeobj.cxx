//#define MODULE_NAME "ddefile"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"
#if 0

#include <ctype.h>
#include <sysdep.h>
#if defined SYSTEM_WIN

#include <stdlib.h>
#include <string.h>

#include <sosstrg0.h>
#include <sosstrng.h>
#include <windows.h>
#include <ddeml.h>

#include <sos.h>
#include <xception.h>
#include <log.h>
#include <sosobj.h>
#include <sosmsg.h>
#include <sosfiltr.h>
#include <sosfact.h>
#include <sosdde.h>

//----------------------------------------------------------------------------------Dde_client

struct Dde_client : Sos_msg_filter, Sos_dde::Conversation, Sos_dde
{
    BASE_CLASS( Sos_msg_filter )

                                Dde_client              ();
                               ~Dde_client              ();

  protected:
    void                        close                   ( Close_mode );
    void                        poke_or_execute         ( const Const_area&, HSZ, uint type );

    void                        xtyp_disconnect         ();
    void                        xtyp_advdata            ( const Hsz&, HDDEDATA, uint );
    void                        xtyp_xact_complete      ( const Hsz&, HDDEDATA, uint, int4 );

    SOS_DECLARE_MSG_DISPATCHER

  private:
    void                       _obj_open_msg            ( Create_msg* );
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_end_msg             ( End_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_get_msg             ( Get_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );

    Sos_object_ptr             _runner;
    uint4                      _request_id;
    uint4                      _poke_id;
    uint4                      _advise_id;              // ?
    String_handle              _item;
    uint                       _format;
    uint                       _format_sos_binary;
    Bool                       _out_busy;
};

//-----------------------------------------------------------------------------Dde_client_descr

struct Dde_client_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char*                 name                    () const    { return "dde"; }
    Bool                        handles_complete_name   () const    { return true; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Dde_client> file = SOS_NEW_PTR( Dde_client() );
        return +file;
    }
};

const Dde_client_descr        _dde_client_descr;
extern const Sos_object_descr& dde_client_descr = _dde_client_descr;

//-----------------------------------------------------------------------Dde_client::Dde_client

Dde_client::Dde_client()
:
    _request_id ( 0 ),
    _poke_id    ( 0 ),
    _format     ( CF_TEXT ),
    _format_sos_binary( 0 ),
    _out_busy   ( false )
{
}

//-----------------------------------------------------------------------Dde_client::Dde_client

void Dde_client::_obj_open_msg( Create_msg* m )
{
    _obj_client_ptr = m->source_ptr();

  //CONVCONTEXT conv_context;
    Sos_string  service_name;
    Sos_string  topic_name;
    Sos_string  format_name = "text";

    const char* p = c_str( m->name() );

    if( *p == '-' )
    {
        if( memcmp( "-format ", p, 8 ) == 0 ) {
            p += 8;
            while( *p == ' ' )  p++;
            const char* p2 = p;
            while( isalnum( *p2 ) || *p2 == '_' )  p2++;
            format_name = as_string( p, p2 - p );
            p = p2;
            while( *p == ' ' )  p++;

            if( format_name == "text"         )  _format = CF_TEXT;
            else
            if( format_name == "bitmap"       )  _format = CF_BITMAP;
            else
            if( format_name == "metafilepict" )  _format = CF_METAFILEPICT;
            else
            if( format_name == "sylk"         )  _format = CF_SYLK;
            else
            if( format_name == "dif"          )  _format = CF_DIF;
            else
            if( format_name == "tiff"         )  _format = CF_TIFF;
            else
            if( format_name == "oemtext"      )  _format = CF_OEMTEXT;
            else
            if( format_name == "dib"          )  _format = CF_DIB;
            else
            if( format_name == "palette"      )  _format = CF_PALETTE;
            else
            if( format_name == "riff"         )  _format = CF_RIFF;
            else
            if( format_name == "wave"         )  _format = CF_WAVE;
            else
            {
                _format = RegisterClipboardFormat( c_str( format_name ) );
                if( !_format )  throw_xc( "MSWIN-RegisterClipboardFormat", c_str( format_name ) );
                _format_sos_binary = RegisterClipboardFormat( "sos_binary" );
            }
        }

        //if( memcmp( "-item ", p, 6 ) == 0 ) {
        //    _item = String_handle( ddeml_instance_ptr(), "" );
        //}
    }

    while( *p == ' ' )  p++;

    int service_name_length = min( position( p, '|' ), position( p, '!' ) );
    if( service_name_length == length( p ) )  throw_xc( "D104" );

    service_name = as_string( p, service_name_length );
    topic_name   = p + service_name_length + 1;

    String_handle service_handle ( ddeml_instance_ptr(), service_name );
    String_handle topic_handle( ddeml_instance_ptr(), topic_name );

    // Vorsicht! SendMessage()!
    _handle = DdeConnect( *ddeml_instance_ptr(), service_handle, topic_handle, 0
                          /*&conv_context*/ );

    if( !_handle )  throw_dde_error( "DdeConnect" );

    _item.assign( ddeml_instance_ptr(), " " );

    obj_created();
}

//----------------------------------------------------------Dde_client::~Dde_client

Dde_client::~Dde_client()
{
    try {
        close( close_error );
    }
    catch( const Xc& x )
    {
        LOG_ERR( x << '\n' );
    }
}

//----------------------------------------------------------------------------Dde_client::close

void Dde_client::close( Close_mode )
{
    if( !_handle )  return;

    BOOL rc = DdeDisconnect( _handle );

    if( !rc )  throw_dde_error( "DdeDisconnect" );
}

//---------------------------------------------------------------------Dde_client::_obj_run_msg

void Dde_client::_obj_run_msg( Run_msg* m )
{
    _runner = m->source_ptr();

    HDDEDATA data_handle;

    data_handle = DdeClientTransaction( 0, 0,
                                        _handle,
                                        _item,
                                        _format,
                                        XTYP_ADVSTART /* | XTYPF_ACKREQ*/,
                                        TIMEOUT_ASYNC,
                                        &_advise_id );

    if( !data_handle )  throw_dde_error( "DdeClientTransaction" );
}

//---------------------------------------------------------------------Dde_client::_obj_end_msg

void Dde_client::_obj_end_msg( End_msg* m )
{
    _obj_client_ptr = m->source_ptr();
    close( close_normal );
    obj_reply_ack();
}

//---------------------------------------------------------------------Dde_client::_obj_get_msg

void Dde_client::_obj_get_msg( Get_msg* m )
{
    if( _request_id )  throw_busy_error();

    _obj_client_ptr = m->source_ptr();

    HDDEDATA data_handle;

    data_handle = DdeClientTransaction( 0, 0,
                                        _handle,
                                        _item,
                                        _format,
                                        XTYP_REQUEST,
                                        TIMEOUT_ASYNC,
                                        &_request_id );

    if( !data_handle )  throw_dde_error( "DdeClientTransaction" );
}

//------------------------------------------------------------------Dde_client::xtyp_disconnect

void Dde_client::xtyp_disconnect()
{
    &Sos_dde::Conversation::xtyp_disconnect;

    if( _runner ) {
        reply( Ack_msg( _runner, this ) );
    }
}

//---------------------------------------------------------------------Dde_client::xtyp_advdata

void Dde_client::xtyp_advdata( const Hsz& item_handle, HDDEDATA data_handle, uint format )
{
    &Sos_dde::Conversation::xtyp_advdata;

    if( _out_busy )  throw_busy_error();

    uint4 length;
    char* ptr = DdeAccessData( data_handle, &length );

    if( !ptr )  throw_dde_error( "DdeAccessData" );

    if( _format == CF_TEXT ) {
       // Länge korrigieren: (DdeGetData() oder DDE-Server liefert falsche Länge)
       length = strlen( ptr );
       if( length  &&  ptr[ length - 1 ] == '\n' )  length--;
       if( length  &&  ptr[ length - 1 ] == '\r' )  length--;
    }

    Data_msg m ( obj_output_ptr(), this, Const_area( ptr, length ) );
    request( &m );
    _out_busy = true;

    DdeUnaccessData( data_handle );
}

//---------------------------------------------------------------------Dde_client::_obj_ack_msg

inline void Dde_client::_obj_ack_msg( Ack_msg* m )
{
    _out_busy = false;
}

//---------------------------------------------------------------Dde_client::xtyp_xact_complete

void Dde_client::xtyp_xact_complete( const Hsz& item_handle, HDDEDATA data_handle, uint format, int4 id )
{
    &Sos_dde::Conversation::xtyp_xact_complete;

    if( id == _poke_id )
    {
        _poke_id = 0;
        reply( Ack_msg( _obj_client_ptr, this ) );
        _obj_client_ptr = 0;
    }
    else
    if( id == _request_id )
    {
        _request_id = 0;

        uint4 length;
        char* ptr = DdeAccessData( data_handle, &length );

        if( !ptr )  throw_dde_error( "DdeAccessData" );

        if( _format == CF_TEXT ) {
            // Länge korrigieren: (DdeGetData() oder DDE-Server liefert falsche Länge)
            length = strlen( ptr );
            if( length  &&  ptr[ length - 1 ] == '\n' )  length--;
            if( length  &&  ptr[ length - 1 ] == '\r' )  length--;
        }
        else
        if( format == _format_sos_binary ) {
            uint l = ( (uint)ptr[0] << 16 )  |  ( (uint)ptr[1] << 8 )  |  ptr[2];
            if( l > length )  obj_reply_error( Xc( "SOS-1138" ) );
            length = l;
            ptr += 3;
        }

        reply( Data_reply_msg( _obj_client_ptr, this, Const_area( ptr, length ) ) );
        _obj_client_ptr = 0;

        DdeUnaccessData( data_handle );
    }
    else
    {
        LOG_ERR( "XTYP_XACT_COMPLETE nicht zuordenbar\n" );
    }
}

//-------------------------------------------------------------------------------------------

inline void Dde_client::_obj_data_msg( Data_msg* m )
{
    _obj_client_ptr = m->source_ptr();

    poke_or_execute( m->data(), _item, XTYP_POKE );
}

//-------------------------------------------------------------------------------------------

void Dde_client::poke_or_execute( const Const_area& record, HSZ item_hsz, uint type )
{
    if( _poke_id )  { obj_busy(); return; }

    void*       data_ptr;
    int4        data_length;
    HDDEDATA    rc;

    if( _format == CF_TEXT
     &&  ( record.length() == 0  ||  record.byte_ptr()[ record.length() - 1 ] != '\0' ) )
    {
        HDDEDATA data_handle = DdeCreateDataHandle( *ddeml_instance_ptr(), 0,
                                                    record.length() + 1, 0,
                                                    item_hsz, _format, 0 );
        DdeAddData( data_handle, (void*)record.ptr(), record.length(), 0 );
        DdeAddData( data_handle, "\0", 1, record.length() );
        data_ptr    = (void*)data_handle;
        data_length = -1;
    }
    else
    if( _format == _format_sos_binary )
    {
        HDDEDATA data_handle = DdeCreateDataHandle( *ddeml_instance_ptr(), 0,
                                                    3 + record.length(), 0,
                                                    item_hsz, _format, 0 );
        Byte l [ 3 ];
        l[ 0 ] = (Byte)( (uint4)record.length() >> 16 );
        l[ 1 ] = (Byte)( record.length() >> 8 );
        l[ 2 ] = (Byte)  record.length();
        DdeAddData( data_handle, l, 3, 0 );
        DdeAddData( data_handle, (void*)record.ptr(), record.length(), 3 );
        data_ptr    = (void*)data_handle;
        data_length = -1;
    }
    else
    {
        data_ptr    = (void*)record.ptr();
        data_length = record.length();
    }
    if( !data_ptr )  throw_dde_error( "DdeCreateDataHandle" );

    rc = DdeClientTransaction( data_ptr, data_length,
                               _handle,
                               item_hsz,
                               _format,
                               type,
                               TIMEOUT_ASYNC,
                               &_poke_id );
    if( rc == (HDDEDATA)FALSE )  throw_dde_error( "DdeClientTransaction" );
}                                       

//-------------------------------------------------------------------------Dde_client::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Dde_client )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( get  )
    SOS_DISPATCH_MSG( open )
    SOS_DISPATCH_MSG( run  )
    SOS_DISPATCH_MSG( end  )
SOS_END_MSG_DISPATCHER


#endif
#endif
