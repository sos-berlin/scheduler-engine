// $Id: xml_msxml.cxx 12891 2007-06-29 12:29:34Z jz $

#include "zschimmer.h"

#ifdef Z_WINDOWS

#include "xml_msxml.h"
#include "file.h"
#include "z_windows.h"

using namespace zschimmer;
using namespace zschimmer::com;


namespace zschimmer {
namespace xml_msxml {
namespace xml {


//----------------------------------------------------------------------------Node_ptr::nodeName_is    

bool Node_ptr::nodeName_is( const char* name ) const
{
    return compare_olechar_with_char( _ptr->nodeName, name ) == 0;
}

//-------------------------------------------------------------------Element_ptr::bool_getAttribute

bool Element_ptr::bool_getAttribute( const string& name, bool deflt ) const
{
    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_bool( attr.value() );
          else  return deflt;
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::int_getAttribute( const string& name, int deflt ) const
{
    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_int( attr.value() );
          else  return deflt;
}

//-------------------------------------------------------------------Element_ptr::uint_getAttribute

int Element_ptr::uint_getAttribute( const string& name, int deflt ) const
{
    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_uint( attr.value() );
          else  return deflt;
}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const char* name, const string& value ) const
{
    HRESULT hr = _ptr->setAttribute( name, value.c_str() );
}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const string& name, int value ) const
{
    setAttribute( name, as_string(value) );
}

//-----------------------------------------------------------------------------Document_ptr::create

void Document_ptr::create()
{ 
    _ptr = NULL;
    Node_ptr::_ptr = NULL;

    HRESULT hr = _ptr.CreateInstance( __uuidof(msxml::DOMDocument) ); 
    if( FAILED(hr) )  throw_com( hr, "CreateInstance", "msxml::DOMDocument" );
    //HRESULT hr = _ptr.CreateInstance( __uuidof(msxml::DOMDocument30) ); 
    //if( FAILED(hr) )  throw_com( hr, "CreateInstance", "msxml::DOMDocument30" );

    Node_ptr::_ptr = _ptr;
}

//----------------------------------------------------------------------------Document_ptr::load_xml

void Document_ptr::load_xml( const string& xml_text )
{
    bool ok = try_load_xml( xml_text );
    if( !ok )  throw_xc( "MSXML", error_text() );
}

//-------------------------------------------------------------------------Document_ptr::error_text

string Document_ptr::error_text()
{
    msxml::IXMLDOMParseErrorPtr error = _ptr->parseError;

    string text = string_from_bstr( error->reason );

    if( text[ text.length()-1 ] == '\n' )  text = string( text.c_str(), text.length() - 1 );
    if( text[ text.length()-1 ] == '\r' )  text = string( text.c_str(), text.length() - 1 );

    text += ", code="   + printf_string( "%08X", error->errorCode );
    text += ", line="   + as_string( error->line );
    text += ", column=" + as_string( error->linepos );

    return text;
}

//--------------------------------------------------------------------------------Document_ptr::xml

string Document_ptr::xml( bool ) const
{
    char   tmp_filename [MAX_PATH];
    int    ret;
    string result;

    ret = GetTempPath( sizeof tmp_filename, tmp_filename );
    if( ret == 0 )  throw_mswin( "GetTempPath" );

    ret = GetTempFileName( tmp_filename, "sos", 0, tmp_filename );
    if( ret == 0 )  throw_mswin( "GetTempFileName" );

    //LOG( "Temporäre Datei " << tmp_filename << " für XML-Antwort\n" );

    try 
    {
        _ptr->save( tmp_filename );
        result = file::string_from_file( tmp_filename );
        unlink( tmp_filename );
    }
    catch( const exception&  ) { unlink( tmp_filename ); throw; }
    catch( const _com_error& ) { unlink( tmp_filename ); throw; }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace xml_msxml
} //namespace zschimmer

#endif
