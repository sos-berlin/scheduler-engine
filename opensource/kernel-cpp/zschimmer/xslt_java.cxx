#include "zschimmer.h"
#if defined Z_USE_JAVAXML

#include "../javaproxy/java__lang__String.h"
#include "xslt_java.h"
#include "log.h"
#include "file.h"

typedef ::javaproxy::java::lang::String StringJ;

using namespace std;
using namespace zschimmer::com;

namespace zschimmer {
namespace xml {

//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "JAVAXSLT-001", "No stylesheet loaded" },
    { "JAVAXSLT-002", "Not parameter syntax NAME=WERT: $1" },
    { "JAVAXSLT-003", "Stylesheet not loadable" },
    { "JAVAXSLT-004", "Error when applying stylesheet \"$1\": $2" },
    { "JAVAXSLT-005", "Only one sort of quote ['\"] in parameter value is supported: $1=$2" },
    { NULL, NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT(xml_javaxml) {
    add_message_code_texts( error_codes );
}

//-----------------------------------------------------------------Xslt_parameters::Xslt_parameters

Xslt_parameters::Xslt_parameters(int)
: 
   _hashMapJ(HashMapJ::new_instance())
{}

//----------------------------------------------------------------Xslt_parameters::~Xslt_parameters
    
Xslt_parameters::~Xslt_parameters()
{}

//------------------------------------------------------------------------Xslt_parameters::allocate

void Xslt_parameters::allocate(int)
{           
    _hashMapJ = HashMapJ::new_instance();
}

//-----------------------------------------------------------------------Xslt_parameters::set_xpath

void Xslt_parameters::set_xpath(int, const string& name, const string& value)
{
    _hashMapJ.put(StringJ(name), StringJ(value));
    //Parameter p;
    //p._name = name;
    //p._value = value;
    //_parameters[i] = p;
}

//----------------------------------------------------------------------Xslt_parameters::set_string

void Xslt_parameters::set_string(int i, const string& name, const string& value)
{
    Z_LOG2("zschimmer", Z_FUNCTION << " " << i << " " << name << "=" << value << "\n");

    string string_value;

    if( value.find( '"' ) != string::npos )
    {
        if( value.find( '\'' ) != string::npos )  throw_xc( "JAVAXSLT-005", name, value );    // Anscheinend kennt libxslt keine Ersatzdarstellung für Anführungszeichen
        string_value = "'" + value + "'";
    }
    else
    {
        string_value = "\"" + value + "\"";
    }

    set_xpath( i, name, string_value );
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet()
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
{}

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

//-----------------------------------------------------------------------Xslt_stylesheet::load_file

void Xslt_stylesheet::load_file( const string& path )
{
    _path = path;   // Auch im Fehlerfall
    _stylesheetJ = CppXsltStylesheetJ::new_instance(Document_ptr(file::string_from_file(path)).ref());
}

//------------------------------------------------------------------------Xslt_stylesheet::load_xml

void Xslt_stylesheet::load( const Document_ptr& stylesheet )
{
    _path = "";
    _stylesheetJ = CppXsltStylesheetJ::new_instance(stylesheet.ref());
}

//---------------------------------------------------------------------------Xslt_stylesheet::apply

Document_ptr Xslt_stylesheet::apply( const Document_ptr& document )
{
    Xslt_parameters no_parameters ( 0 );
    return apply( document, no_parameters );
}

//---------------------------------------------------------------------------Xslt_stylesheet::apply

Document_ptr Xslt_stylesheet::apply( const Document_ptr& document, const Xslt_parameters& parameters )
{
    if(!_stylesheetJ)  throw_xc( "JAVAXSLT-001", _path );

    return _stylesheetJ.apply(document.ref(), parameters._hashMapJ);
}

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace

#endif
