// $Id: xslt_libxslt.cxx 13691 2008-09-30 20:42:20Z jz $

// Dokumentation von libxml2 in http://xmlsoft.org/html/libxml-lib.html

#include "zschimmer.h"
#include "xslt_libxslt.h"
#include "log.h"

#include "../3rd_party/libxml2/libxml.h"
#include "../3rd_party/libxml2/include/libxml/xmlmemory.h"
#include "../3rd_party/libxml2/include/libxml/xmlIO.h"

#include "../3rd_party/libxslt/libxslt/xslt.h"
#include "../3rd_party/libxslt/libxslt/xsltInternals.h"
#include "../3rd_party/libxslt/libxslt/transform.h"
#include "../3rd_party/libxslt/libxslt/xsltutils.h"



using namespace std;
using namespace zschimmer::com;


namespace zschimmer {
namespace xml {
namespace libxml2 {

//-------------------------------------------------------------------------Libxml_output_buffer_ptr
/*
struct Libxml_output_buffer_ptr
{
    Libxml_output_buffer_ptr() 
    : 
        _ptr(NULL) 
    {
    }


    ~Libxml_output_buffer_ptr()
    {
        if( _ptr )  xmlOutputBufferClose( _ptr );
    }


    operator xmlOutputBufferPtr () 
    { 
        return _ptr; 
    }


    xmlOutputBufferPtr         _ptr;
};
*/
//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "LIBXSLT-001", "No stylesheet loaded" },
    { "LIBXSLT-002", "Not parameter syntax NAME=WERT: $1" },
    { "LIBXSLT-003", "Stylesheet not loadable" },
    { "LIBXSLT-004", "Error when applying stylesheet \"$1\": $2" },
    { "LIBXSLT-005", "Only one sort of quote ['\"] in parameter value is supported: $1=$2" },
    { NULL         , NULL }
};

    /*
    { "LIBXSLT-001", "Kein Stylesheet geladen" },
    { "LIBXSLT-002", "Parametersyntax NAME=WERT nicht eingehalten: $1" },
    { "LIBXSLT-003", "Stylesheet nicht ladbar" },
    { "LIBXSLT-004", "Fehler bei der Anwendung des Stylesheets auf das XML-Dokument" },
    */
//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( xml_libxslt )
{
    add_message_code_texts( error_codes );
}

//--------------------------------------------------------------------------Xslt_parameters::create
/*
Xslt_parameters Xslt_parameters::create( const std::vector<string>& parameters )
{
    Xslt_parameters result ( parameters.size() );

    for( uint i = 0; i < parameters.size(); i++ )
    {
        const string& parameter = parameters[i];
        size_t equal = parameter.find( '=' );
        if( equal == string::npos )  throw_xc( "LIBXSLT-002", parameter );
        result.set( i, parameter.substr( 0, equal ), parameter.substr( equal + 1 ) );
    }

    return result;
}
*/
//-----------------------------------------------------------------Xslt_parameters::Xslt_parameters

Xslt_parameters::Xslt_parameters( int count )
:                
    _n(0),
    _array( NULL )
{
    allocate( count );
}

//----------------------------------------------------------------Xslt_parameters::~Xslt_parameters
    
Xslt_parameters::~Xslt_parameters()
{
    close();
}

//---------------------------------------------------------------------------Xslt_parameters::close

void Xslt_parameters::close()
{
    if( _array )
    {
        for( int i = 0; i < 2*_n + 1; i++ )  free( (void*)_array[i] );
        delete[] _array;
        _array = NULL;
        _n = 0;
    }
}

//------------------------------------------------------------------------Xslt_parameters::allocate

void Xslt_parameters::allocate( int count )
{           
    close();

    if( count > 0 )
    {
        _n = count;
        _array = new const char*[ 2*count + 1 ];
        for( int i = 0; i < 2*count + 1; i++ )  _array[i] = NULL;
    }
}

//-----------------------------------------------------------------------Xslt_parameters::set_xpath

void Xslt_parameters::set_xpath( int i, const string& name, const string& value )
{
    assert( i >= 0  &&  i < _n );

    if( _array[ 2*i ] )  free( (void*)_array[ 2*i ] );   
    _array[ 2*i ] = strdup( (const char*)Utf8_string( name ).utf8() );

    if( _array[ 2*i+1 ] )  free( (void*)_array[ 2*i+1 ] ); 
    _array[ 2*i+1 ] = strdup( (const char*)Utf8_string( value ).utf8() );
}

//----------------------------------------------------------------------Xslt_parameters::set_string

void Xslt_parameters::set_string( int i, const string& name, const string& value )
{
    Z_LOG2( "zschimmer", Z_FUNCTION << " " << i << " " << name << "=" << value << "\n" );

    string string_value;

    if( value.find( '"' ) != string::npos )
    {
        if( value.find( '\'' ) != string::npos )  throw_xc( "LIBXSLT-005", name, value );    // Anscheinend kennt libxslt keine Ersatzdarstellung für Anführungszeichen
        string_value = "'" + value + "'";
    }
    else
    {
        string_value = "\"" + value + "\"";
    }

    set_xpath( i, name, string_value );
}

//----------------------------------------------------------slt_stylesheet_ptr::Xslt_stylesheet_ptr

Xslt_stylesheet_ptr::Xslt_stylesheet_ptr( _xsltStylesheet* ptr )
:
    _ptr( ptr )
{
}

//---------------------------------------------------------slt_stylesheet_ptr::~Xslt_stylesheet_ptr

Xslt_stylesheet_ptr::~Xslt_stylesheet_ptr()
{
    assign( NULL );
}

//----------------------------------------------------------slt_stylesheet_ptr::Xslt_stylesheet_ptr

Xslt_stylesheet_ptr& Xslt_stylesheet_ptr::operator = ( _xsltStylesheet* ptr )
{
    assign( ptr );
    return *this;
}

//----------------------------------------------------------slt_stylesheet_ptr::Xslt_stylesheet_ptr

void Xslt_stylesheet_ptr::assign( _xsltStylesheet* ptr )
{
    if( _ptr )
    {
        xsltFreeStylesheet( _ptr );
        _ptr = NULL;

        //? xsltCleanupGlobals();
	//? xmlCleanupParser();
    }

    _ptr = ptr;
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet( Xslt_stylesheet_ptr* stylesheet_ptr )
: 
    _stylesheet_ptr(stylesheet_ptr)
{
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet( const string& xml_or_filename )
{
    if( is_xml( xml_or_filename ) )
    {
        load( Document_ptr( xml_or_filename ) );
    }
    else
    {
        load_file( xml_or_filename );
    }
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet( const BSTR xml_or_filename_bstr )
{
    if( is_xml( xml_or_filename_bstr ) )
    {
        load( Document_ptr( xml_or_filename_bstr ) );
    }
    else
    {
        load_file( string_from_bstr( xml_or_filename_bstr ) );
    }
}

//----------------------------------------------------------------Xslt_stylesheet::~Xslt_stylesheet

Xslt_stylesheet::~Xslt_stylesheet()
{
    release();
}

//-------------------------------------------------------------------------Xslt_stylesheet::release

void Xslt_stylesheet::release()
{
    _stylesheet_ptr = NULL;
}

//--------------------------------------------------------------------------Xslt_stylesheet::is_xml

bool Xslt_stylesheet::is_xml( const string& str )
{
    return string_begins_with( str, "<" );
}

//--------------------------------------------------------------------------Xslt_stylesheet::is_xml

bool Xslt_stylesheet::is_xml( const BSTR bstr )
{
    return bstr && bstr[0] == '<';
}

//-----------------------------------------------------------------Xslt_stylesheet::prepare_parsing

void Xslt_stylesheet::prepare_parsing()
{
    xmlSubstituteEntitiesDefault( 1 );
    xmlLoadExtDtdDefaultValue = 1;
}

//-----------------------------------------------------------------------Xslt_stylesheet::load_file

void Xslt_stylesheet::load_file( const string& path )
{
    _stylesheet_ptr = NULL;

    _path = path;   // Auch im Fehlerfall

    prepare_parsing();
    Activate_error_text activate_error_text ( this );

    _xsltStylesheet* p = xsltParseStylesheetFile( Utf8_string( path ).utf8() );
    if( !p )  throw_xc( "LIBXSLT-003", "xsltParseStylesheetFile", _path, _error_text );

    _stylesheet_ptr = Z_NEW( Xslt_stylesheet_ptr( p ) );
}

//------------------------------------------------------------------------Xslt_stylesheet::load_xml

void Xslt_stylesheet::load( const Document_ptr& stylesheet )
{
    _stylesheet_ptr = NULL;
    _path = "";

    prepare_parsing();
    Activate_error_text activate_error_text ( this );

    _xsltStylesheet* p = xsltParseStylesheetDoc( stylesheet.ptr() );
    if( !p )  throw_xc( "LIBXSLT-003", "xsltParseStylesheetDoc", _path, _error_text );

    _stylesheet_ptr = Z_NEW( Xslt_stylesheet_ptr( p ) );
}

//---------------------------------------------------------------------------Xslt_stylesheet::apply

Document_ptr Xslt_stylesheet::apply( const Document_ptr& document )
{
    Xslt_parameters no_parameters ( 0 );    // Für gcc 3.4.3
    return apply( document, no_parameters );
}

//---------------------------------------------------------------------------Xslt_stylesheet::apply

Document_ptr Xslt_stylesheet::apply( const Document_ptr& document, const Xslt_parameters& parameters )
{
    if( !_stylesheet_ptr )  throw_xc( "LIBXSLT-001", _path );

    Document_ptr    result;
    Activate_error_text activate_error_text ( this );

    if( Log_ptr log = "" )  // Aufruf von xsltApplyStylesheet() immer protokollieren, denn es kann abstürzen.
    {
        log << "xsltApplyStylesheet( \"" << _path << "\"";
        if( parameters._array ) for( const char** p = parameters._array; *p; p += 2 )  log << ", " << p[0] << "=" << p[1];
        *log << " )\n" << flush;
    }

    result = xsltApplyStylesheet( _stylesheet_ptr->_ptr, document.ptr(), parameters._array );
    if( !result )  throw_xc( "LIBXSLT-004", "xsltApplyStylesheet", _path, _error_text );

    return result;
}

//------------------------------------------------------------Xslt_stylesheet::write_result_to_file

void Xslt_stylesheet::write_result_to_file( const Document_ptr& result_document, const string& filename )
{
  //xmlIndentTreeOutput = 1;

    int compression = 0;
    xsltSaveResultToFilename( filename.c_str(), result_document.ptr(), _stylesheet_ptr->_ptr, compression );

    /*
    File file ( filename, "w" );
    xsltSaveResultToFd( file, result_document._ptr, _ptr );
    file.release();
    */
}

//-----------------------------------------------------------------Xslt_stylesheet::xml_from_result
/*
string Xslt_stylesheet::xml_from_result( const Document_ptr& result_document )
{
    string                  result;
    Libxml_output_buffer_ptr    buffer;


    int byte_count = xsltSaveResultTo( buffer, result_document.ptr(), _ptr );
    if( byte_count < 0 )  throw_xc( "LIBXSLT-xsltSaveResultToString" );

    return sd( buffer );
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace libxml2
} //namespace xml
} //namespace zschimmer

//-------------------------------------------------------------------------------------------------
