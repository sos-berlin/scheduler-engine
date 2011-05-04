//#define MODULE_NAME "ddefile"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#if 0
#include "../kram/sysdep.h"
#if defined SYSTEM_WIN


#include <stdlib.h>
#include <string.h>
//#include <cstring.h>

#if defined SYSTEM_WIN32
//#   if defined SYSTEM_MICROSOFT
//#       include <afx.h>
//#   else
#       include <windows.h>  // bc5.00
//#   endif
#endif

#include <ddeml.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../kram/sosdde.h"
#include "../kram/sosprof.h"
#include "ddefile.h"


namespace sos {

//----------------------------------------------------------------------statics

struct Dde_file_type : Abs_file_type
{
                                Dde_file_type           () : Abs_file_type() {};

    virtual const char*         name                    () const { return "dde"; }
  //virtual const char*         alias_name              () const { return ""; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Dde_file> f = SOS_NEW_PTR( Dde_file );
        return +f;
    }
};

const Dde_file_type   _dde_file_type;
const Abs_file_type&   dde_file_type = _dde_file_type;

//-----------------------------------------------------------Dde_file::Dde_file

Dde_file::Dde_file()
:
    _conv_handle ( 0 )
{
    _init();
}

//----------------------------------------------------------Dde_file::~Dde_file

Dde_file::~Dde_file()
{
    close( close_error );
}

//---------------------------------------------------------------Dde_file::_init

void Dde_file::_init()
{
    _key_pos = 0;
    _key_len = 0;
}

//---------------------------------------------------------------Dde_file::open

void Dde_file::open( const char* filename, Open_mode, const File_spec& file_spec )
{
    Sos_string  service_name;
    Sos_string  topic_name;

    _timeout_ms = read_profile_uint( "", "dde", "timeout", 120 ) * 1000L;//120 * 1000;

    Sos_option_iterator opt ( filename );

    for( opt; !opt.end(); opt.next() ) {
        if( opt.with_value( "timeout" ) )  _timeout_ms = 1000L * as_int( opt.value() );
        else
        if( opt.param() ) break;
        else throw_sos_option_error( opt );
    }

    const char* fn = c_str( opt.rest() );

    const char* p = (const char*) strchr( fn, '!' );
    if( !p )  throw_syntax_error( "D104" );  //raise( "FILENAME", "D104" );

    append( &service_name, fn, p - fn );
    topic_name = p + 1;

    _conv_handle = DdeConnect( *ddeml_instance_ptr(),
                               String_handle( ddeml_instance_ptr(), service_name ),
                               String_handle( ddeml_instance_ptr(), topic_name ),
                               (CONVCONTEXT*)0 );

    if( !_conv_handle )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr() ), "DdeConnect", c_str( service_name ), c_str( topic_name ) );//throw_dde_error( "DdeConnect", c_str( service_name ), c_str( topic_name ) );

    _key_pos = file_spec.key_specs().key_position();
    _key_len = file_spec.key_specs().key_length();
}

//--------------------------------------------------------------Dde_file::close

void Dde_file::close( Close_mode )
{
    if( !_conv_handle )  return;

    BOOL rc = DdeDisconnect( _conv_handle );
    _conv_handle = 0;

    if( !rc )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr() ), "DdeDisconnect" ); //throw_dde_error( "DdeDisconnect" );

    _init();
}

//-------------------------------------------------------------Dde_file::rewind

void Dde_file::rewind( Key::Number )
{
    // ?????
}

//---------------------------------------------------------Dde_file::get_record

void Dde_file::get_record( Area& area )
{
    get_record_key( area, Const_area( "" ) );
}

//---------------------------------------------------------Dde_file::get_record_key

void Dde_file::get_record_key( Area& area, const Key& key )
{
    DWORD    result = 0;
    HDDEDATA data_handle;

    char        key_buffer [ 256 + 1 ];                   // 0-Byte anhängen
    const char* p          = (const char*) memchr( key.ptr(), ' ', sizeof key_buffer );
    int         key_length = p? p - key.char_ptr() : sizeof key_buffer - 1;

    memcpy( key_buffer, key.ptr(), key_length );
    key_buffer[ key_length ] = '\0';
    String_handle key_handle ( ddeml_instance_ptr(), key_buffer );

    data_handle = DdeClientTransaction( 0, 0,
                                        _conv_handle,
                                        key_handle,
                                        CF_TEXT,
                                        XTYP_REQUEST,
                                        _timeout_ms,
                                        &result );

    if( data_handle == (HDDEDATA)FALSE )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr() ), "DdeClientTransaction", key_buffer );

    uint length = DdeGetData( data_handle, 0, 0, 0 );
    area.allocate_min( length );
    DdeGetData( data_handle, (Dde_data*)area.ptr(), area.size(), 0 );

    // Länge korrigieren: (DdeGetData() oder DDE-Server liefert falsche Länge)
    length = strlen( area.char_ptr() );
    if( length >= 2 ) {                   // CR LF entfernen:
        if( area.char_ptr()[ length - 1 ] == '\n' )  length--;
        if( area.char_ptr()[ length - 1 ] == '\r' )  length--;
    }

    area.length( length );

    DdeFreeDataHandle( data_handle );
}

//-------------------------------------------------------------------------------------------

void Dde_file::store( const Const_area& area )
{
    store_key( area, Const_area( "" ));
}

//-------------------------------------------------------------------------------------------

void Dde_file::store_key( const Const_area& area, const Key& key )
{
  //const char* key_ptr = key.char_ptr();
    char        key_buffer [ file_max_key_length + 1 ];
  //const char* p = (const char*) memchr( key_ptr, ' ', sizeof key_buffer );      // Blank gegen '\0' tauschen
  //int         key_length = p? p - key_ptr : sizeof key_buffer - 1;

    memcpy( key_buffer, key.ptr(), key.length() );
    key_buffer[ key.length() ] = '\0';

    if( strcmp( key_buffer, "*EXECUTE" ) == 0 ) {
        _dde_poke_or_execute( area, 0, XTYP_EXECUTE );
    } else {
        String_handle key_handle ( ddeml_instance_ptr(), key_buffer );
        _dde_poke_or_execute( area, key_handle, XTYP_POKE );
    }
}

//-------------------------------------------------------------------------------------------

void Dde_file::_dde_poke_or_execute( const Const_area& area, HSZ item_hsz, uint type )
{
    DWORD       result = 0;
    HDDEDATA    rc;

    HDDEDATA data_handle = DdeCreateDataHandle( *ddeml_instance_ptr(),
                                                0, area.length() + 1, 0,
                                                item_hsz,
                                                CF_TEXT,
                                                0 );

    DdeAddData( data_handle, (Dde_data*) area.ptr(), area.length(), 0             );
    DdeAddData( data_handle, (Dde_data*) "\0"      , 1            , area.length() );

    rc = DdeClientTransaction( (Dde_data*)data_handle, -1,
                               _conv_handle,
                               item_hsz,
                               CF_TEXT,
                               type,
                               _timeout_ms,         // Max. Wartezeit in Millisekunden
                               &result );
    if( rc == (HDDEDATA)FALSE )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr() ), "DdeClientTransaction" );
}

/*22.12.97
Record_position Dde_file::key_position( Key::Number )
{
    return _key_position;
}

Record_length Dde_file::key_length( Key::Number )
{
    return _key_length;
}
*/
//----------------------------------------------------------Dde_file::put_record

void Dde_file::put_record( const Const_area& area )
{
    store( area );
}


//------------------------------------------------------Dde_file::norm_filename

/*
void norm_filename ( char* input_name,
                     char  normed_name[ _MAX_PATH ] )
{
   exceptions
}
*/

} //namespace sos

#endif
#endif
