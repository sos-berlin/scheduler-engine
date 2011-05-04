#if 0

#define MODULE_NAME "ddefile"
#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <sysdep.h>
#if defined SYSTEM_WIN


#include <stdlib.h>
#include <string.h>

#include <sosstrng.h>
#include <ddeml.h>

#include <sos.h>
#include <xception.h>
#include <sosdde.h>
#include <ddefile.h>

//----------------------------------------------------------------------statics

struct Dde_file_type : Abs_file_type
{
                                Dde_file_type           () : Abs_file_type() {};

    virtual const char*         name                    () const { return "dde"; }
  //virtual const char*         alias_name              () const { return ""; }

    virtual Bool                is_record_file          ()  const { return true; }
    virtual Bool                is_indexed_file         ()  const { return true; }

    virtual Abs_file_base*      create_base_file        () const { return new Dde_file(); }
    virtual Abs_record_file*    create_record_file      () const { return new Dde_file(); }
    virtual Abs_indexed_file*   create_indexed_file     () const { return new Dde_file(); }
};

static Dde_file_type _Dde_file_type;

const Abs_file_type& Dde_file::static_file_type()
{
    return _Dde_file_type;
}


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
    _key_position = 0;
    _key_length   = 0;
}

//---------------------------------------------------------------Dde_file::open

void Dde_file::open( const char* filename, Open_mode, const File_spec& file_spec )
{
    string  service_name;
    string  topic_name;

    const char* p = (const char*) strchr( filename, '!' );
    if( !p )  raise( "FILENAME", "D104" );

    service_name.append( filename, 0, p - filename );
    topic_name = p + 1;

    _conv_handle = DdeConnect( *ddeml_instance_ptr(),
                               String_handle( ddeml_instance_ptr(), service_name ),
                               String_handle( ddeml_instance_ptr(), topic_name ),
                               (CONVCONTEXT*)0 );

    if( !_conv_handle )  throw Dde_error( *ddeml_instance_ptr(), "DdeConnect" );

    _key_position = file_spec.key_specs().key_position();
    _key_length   = file_spec.key_specs().key_length();

  exceptions
}

//--------------------------------------------------------------Dde_file::close

void Dde_file::close( Close_mode )
{
    if( !_conv_handle )  return;

    BOOL rc = DdeDisconnect( _conv_handle );
    _conv_handle = 0;

    if( !rc )  throw Dde_error( *ddeml_instance_ptr(), "DdeDisconnect" );

    _init();

  exceptions
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

    data_handle = DdeClientTransaction( 0, 0,
                                        _conv_handle,
                                        String_handle( ddeml_instance_ptr(), key_buffer ),
                                        CF_TEXT,
                                        XTYP_REQUEST,
                                        120000l,                 // 120s Wartezeit
                                        &result );

    if( data_handle == (HDDEDATA)FALSE )  throw Dde_error( *ddeml_instance_ptr(), "DdeClientTransaction" );

    uint length = DdeGetData( data_handle, 0, 0, 0 );
    area.allocate_min( length );
    DdeGetData( data_handle, area.ptr(), area.size(), 0 );

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
        _dde_poke_or_execute( area, String_handle( ddeml_instance_ptr(), key_buffer ), XTYP_POKE );
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

    DdeAddData( data_handle, (void*) area.ptr(), area.length(), 0             );
    DdeAddData( data_handle, "\0"              , 1            , area.length() );

    rc = DdeClientTransaction( (void*)data_handle, -1,
                               _conv_handle,
                               item_hsz,
                               CF_TEXT,
                               type,
                               120000l,         // Max. Wartezeit in Millisekunden
                               &result );
    if( rc == (HDDEDATA)FALSE )  throw Dde_error( *ddeml_instance_ptr(), "DdeClientTransaction" );
}

/*28.12.97
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
BEGIN
    store( area );
END


//------------------------------------------------------Dde_file::norm_filename

/*
void norm_filename ( char* input_name,
                     char  normed_name[ _MAX_PATH ] )
{
   exceptions
}
*/


#endif
#endif
