// $Id: xml_libxml2.cxx 14207 2011-03-31 11:26:18Z jz $

// Dokumentation von libxml2 in http://xmlsoft.org/html/libxml-lib.html

#include "zschimmer.h"
#include "xml_libxml2.h"
#include "mutex.h"
#include "z_com.h"
#include "threads.h"
#include "log.h"
#include "string_list.h"

#include <stdarg.h>

#include "../3rd_party/libxml2/libxml.h"
#include "../3rd_party/libxml2/include/libxml/xmlmemory.h"
#include "../3rd_party/libxml2/include/libxml/xmlIO.h"
#include "../3rd_party/libxml2/include/libxml/xpath.h"
#include "../3rd_party/libxml2/include/libxml/xmlschemas.h"


using namespace std;
using namespace zschimmer::com;


namespace zschimmer {
namespace xml {
namespace libxml2 {

//-------------------------------------------------------------------------------------------static

static Mutex                    libxml_parser_mutex ( "libxml_parser_mutex" );
static string                   static_parser_messages;
//static Thread_data<Libxml2_thread_data> thread_data;

//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "LIBXML2-001", "Error in $1" },
    { "LIBXML2-002", "Node $1 expected, instead of $2:$3" },
    { "LIBXML2-003", "Missing attribute $1 in <$2>" },
    { "LIBXML2-004", "XML document is not valid: $1" },
    { "LIBXML2-005", "Missing XML element <$1>" },
    { "LIBXML2-006", "XML schema not loaded" },
    { "LIBXML2-007", "XML document does not conform to schema" },
    { "LIBXML2-008", "No element for x-path $1" },
    { "LIBXML2-009", "Invalid character for XML: U+$1, '$1'" },
    { "LIBXML2-010", "Empty XML document" },
    { NULL         , NULL }
};

/*
    { "LIBXML2-001", "Fehler bei $1" },
    { "LIBXML2-002", "Knoten $1 erwartet, an der Stelle ist aber $2: $3" },
    { "LIBXML2-003", "Attribut $1 fehlt" },
    { "LIBXML2-004", "XML-Dokument ist nicht valide: $1" },
    { "LIBXML2-005", "XML-Element <$1> fehlt" },
    { "LIBXML2-006", "XML-Schema ist nicht geladen" },
    { "LIBXML2-007", "XML-Dokument entspricht nicht dem Schema" },
*/
//-------------------------------------------------------------------------------------------Z_INIT

static struct Xml_libxml2_static
{
    Xml_libxml2_static()
    {
        add_message_code_texts( error_codes );
    }

    ~Xml_libxml2_static()
    {
        // Kein Log, DLL mit Log kann bereits entladen sein!  Z_LOG( "xmlCleanupParser()\n" );
        xmlCleanupParser();
    }
}
__static__;

//----------------------------------------------------------------------------string_from_DOMString
/*
inline string string_from_DOMString( const xmlChar* s )
{
    return s? (const char*)s : "";
}
*/
//-------------------------------------------------------------------------------------------sd_free

string sd_free( xmlChar* s )
{
    string result = sd( s );
    if( s )  xmlFree( s );
    return result;
}

//----------------------------------------------------------------------------DOMString_from_string
/*
inline const xmlChar* DOMString_from_string( const string& s )
{
    return (const xmlChar*)s.c_str();
}
*/
//-----------------------------------------------------------------------------------------------ds
/*
inline const xmlChar* ds( const string& s )
{
    //return (const xmlChar*)s.c_str();
}
*/
//-----------------------------------------------------------------------------------------------ds
/*
inline const xmlChar* ds( const char* s )
{
    return (const xmlChar*)( s? s : "" );
}
*/
//----------------------------------------------------------------------------------xml_print_error

static void xml_print_error( void* context, const char* format, ... )
{
    char buf[1024];
    va_list args;

    va_start( args, format );
    int ret = _vsnprintf( buf, sizeof(buf), format, args ); 
    va_end( args );

    if( ret > 0 )  ((Libxml2_error_text*)context)->_error_text.append( buf, ret );
    //if( ret > 0 )  thread_data->_error_text.append( buf, ret );
}

//---------------------------------------------------------Activate_error_text::Activate_error_text

Activate_error_text::Activate_error_text( Libxml2_error_text* object )
{
    object->_error_code = "";
    object->_error_text = "";
    xmlSetGenericErrorFunc( object, (xmlGenericErrorFunc)xml_print_error );
}

//--------------------------------------------------------Activate_error_text::~Activate_error_text

Activate_error_text::~Activate_error_text()
{
    xmlSetGenericErrorFunc( NULL, NULL );
}

//-----------------------------------------------------------------------------------append_to_utf8

inline void append_to_utf8( string* utf8, char character )
{
    if( (unsigned char)character < 0x80 )
    {
        *utf8 += (char)character;
    }
    else
    {
        *utf8 += 0xC0 + ( (unsigned char)character >> 6   );
        *utf8 += 0x80 + ( (unsigned char)character & 0x3F );
    }
}

//--------------------------------------------------------------------------Utf8_string::set_latin1

void Utf8_string::set_latin1( const char* str, size_t length )
{
    _utf8 = "";
    _utf8.reserve( length + length / 10 );        // 10% Umlaute angenommen

    const char* s     = str;
    const char* s_end = s + length;

    while(1)
    {
        const char* s0 = s;
        while( s < s_end  &&  allowed_xml_ascii_characters[ (unsigned char)*s ] )  s++;
        _utf8.append( s0, s - s0 );

        if( s == s_end )  break;

        if( (unsigned char)*s < 0xA0 )
        {
            //if( character_handling == char_strict )  throw_xc( "LIBXML2-009", *s, string_from_hex( io::Char_sequence( s, 1 ) ) );
            append_to_utf8( &_utf8, unicode_substition_character );
            s++;
        }
        else
        {
            do
            {
                _utf8 += 0xC0 + ( (unsigned char)*s >> 6   );
                _utf8 += 0x80 + ( (unsigned char)*s & 0x3F );
                ++s;
            }
            while( s < s_end  &&  (unsigned char)*s >= 0xA0 );
        }
    }
}

//------------------------------------------------------------------------------------Print_context

struct Print_context
{
    static void static_print( void* context, const char *format_msg, ... )
    {
        va_list list;
        va_start( list, format_msg );

        ((Print_context*)context)->print( format_msg, list );

        va_end( list );
    }

    void print( const char* format_msg, va_list list )
    {
        vfprintf( stderr, format_msg, list );

        char buffer [1024];
        _vsnprintf( buffer, sizeof buffer, format_msg, list );
        _messages += buffer;
    }

    string  _messages;
};

//-----------------------------------------------------------------------------------------------sd

string sd( const xmlChar* utf8 )
{
    string result;

    if( utf8 )
    {
        int char_count = xmlUTF8Strlen( utf8 );
        // Optimierung, wenn Länge von utf8 bekannt: if( char_count == utf8.length() )  return string( utf8 );  // Nur ASCII-Zeichen
        result.reserve( char_count );

        const xmlChar* u = utf8;

        while( *u )
        {
            const xmlChar* u0 = u;
            while( *u != 0  &&  *u < 0x80 )  u++;
            result.append( (const char*)u0, u - u0 );

            while( *u >= 0x80 )
            {
                uint c = 0;

                if( *u < 0x80 )  c += *u++;
                else
                if( *u < 0xE0 )  c += ( ( u[0] & ~0xC0 ) <<  6 ) |   ( u[1] & 0x3F )                                                     ,  u += 2;
                else
                if( *u < 0xF0 )  c += ( ( u[0] & ~0xE0 ) << 12 ) | ( ( u[1] & 0x3F ) <<  6 ) |   ( u[2] & 0x3F )                         ,  u += 3;
                else
                                 c += ( ( u[0] & ~0xF0 ) << 18 ) | ( ( u[1] & 0x3F ) << 12 ) | ( ( u[2] & 0x3F ) << 6 ) | ( u[3] & 0x3F ),  u += 4;

                result += c <= 0xFF? (char)c : '¿';
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------name_of_node_type

string name_of_node_type( const NodeType& n )
{
    switch( n )
    {
        case ELEMENT_NODE                    : return "ELEMENT_NODE";                   break;
        case ATTRIBUTE_NODE                  : return "ATTRIBUTE_NODE";                 break;
        case TEXT_NODE                       : return "TEXT_NODE";                      break;
        case CDATA_SECTION_NODE              : return "CDATA_SECTION_NODE";             break;
        case ENTITY_REFERENCE_NODE           : return "ENTITY_REFERENCE_NODE";          break;
        case ENTITY_NODE                     : return "ENTITY_NODE";                    break;
        case PROCESSING_INSTRUCTION_NODE     : return "PROCESSING_INSTRUCTION_NODE";    break;
        case COMMENT_NODE                    : return "COMMENT_NODE";                   break;
        case DOCUMENT_NODE                   : return "DOCUMENT_NODE";                  break;
        case DOCUMENT_TYPE_NODE              : return "DOCUMENT_TYPE_NODE";             break;
        case DOCUMENT_FRAGMENT_NODE          : return "DOCUMENT_FRAGMENT_NODE";         break;
        case NOTATION_NODE                   : return "NOTATION_NODE";                  break;
                                     default : return as_string( (int)n ); 
    }
};

//----------------------------------------------------------------------------------------sax_print
// Wird mit gesperrtem Mutex libxml_parser_mutex gerufen.
// Schnell raus hier!
/*
static void sax_print( void*, const char *format_msg, ... )
{
    va_list list;

    va_start( list, format_msg );

    vfprintf( stderr, format_msg, list );

    char buffer [1024];
    int ret = _vsnprintf( buffer, sizeof buffer, format_msg, list );
    static_parser_messages += buffer;

    va_end( list );
}
*/
//-----------------------------------------------------------------------Document_ptr::Document_ptr
/* nicht geprüft
Document_ptr::Document_ptr( const Element_ptr& root_to_clone )
{
    create();
    appendChild( clone( root_to_clone ) );
}
*/
//----------------------------------------------------------------------------Document_ptr::release

void Document_ptr::release()
{ 
    if( _ptr ) 
    {
        if( --ptr()->zschimmer_ref_count == 0 )  
        {
            xmlFreeDoc( ptr() );
            _ptr = NULL; 
        }
    }
}

//-----------------------------------------------------------------------------Document_ptr::assign

void Document_ptr::assign( xmlDoc* doc )
{ 
    release(); 
    _ptr = (xmlNode*)doc;  
    if( doc  )  doc->zschimmer_ref_count++; 
}

//-----------------------------------------------------------------------------Document_ptr::create

Document_ptr& Document_ptr::create()
{ 
    release();

    _ptr = (xmlNode*)xmlNewDoc( (const xmlChar*)"1.0" );  
    if(!_ptr)  throw_xc( "LIBXML2-001", "xmlNewDoc()" );  
    ptr()->zschimmer_ref_count = 1;        // Erweiterung von struct xmlDoc von mir, Joacim Zschimmer.

    return *this;
}

//-----------------------------------------------------------------------Document_ptr::try_load_xml

bool Document_ptr::try_load_xml( const string& xml_text, const string& encoding )
{
    xmlDocPtr doc = NULL;

    release();

    if( xml_text == "" )  
    {
        _error_code = "LIBXML2-010";
    }
    else
    {
        Z_MUTEX( libxml_parser_mutex )
        {
            xmlKeepBlanksDefault(0);     // Das ist ja eine statische Funktion!
            xmlLineNumbersDefault( 1 );        // Zeilennummern zählen

            Activate_error_text activate_error_text ( this );

            //xmlParserCtxtPtr parser_context = xmlCreateDocParserCtxt( (xmlChar*)xml_text.c_str() );
            //xmlFreeParserCtxt( parser_context );
            doc = xmlReadMemory( xml_text.data(), xml_text.length(), "", 
                                 encoding == ""? NULL : encoding.c_str(), 
                                 XML_PARSE_DTDLOAD |    // load the external subset
                                 XML_PARSE_DTDATTR |    // default DTD attributes
                                 XML_PARSE_DTDVALID     // validate with the DTD
                             //? XML_PARSE_NOBLANKS |   // remove blank nodes
                             //? XML_PARSE_XINCLUDE |   // Implement XInclude substitition
                               );

            //doc = xmlParseDoc( (xmlChar*)xml_text.c_str() );
            _ptr = (xmlNode*)doc;

            xmlSetGenericErrorFunc( NULL, NULL );
            //xmlCleanupParser();
        }
    }


    if( !_ptr )  return false;



    ptr()->zschimmer_ref_count = 1;        // Erweiterung von struct xmlDoc von mir, Joacim Zschimmer.


    // VALIDIERUNG

    if( doc->intSubset || doc->extSubset )
    {
        Print_context print_context;
        xmlValidCtxt validation_context;  memset( &validation_context, 0, sizeof validation_context );

        validation_context.userData = (void*)                  &print_context;
        validation_context.error    = (xmlValidityErrorFunc)   Print_context::static_print;
        validation_context.warning  = (xmlValidityWarningFunc) Print_context::static_print;

        int ok = xmlValidateDocument( &validation_context, doc );
        if( !ok )  throw_xc( "LIBXML2-004", print_context._messages );
    }


    // Gegen das vielleicht referenzierte Schema wird nicht validiert

    return true;
}

//-----------------------------------------------------------------------Document_ptr::try_load_xml

bool Document_ptr::try_load_xml( const BSTR xml_text_bstr )
{
    return try_load_xml( string_from_bstr( xml_text_bstr ) );
}

//---------------------------------------------------------------------------Document_ptr::load_xml

void Document_ptr::load_xml( const string& xml_text, const string& encoding )
{
    bool ok = try_load_xml( xml_text, encoding );
    if( !ok )  
    {
        if( _error_code != "" )  throw_xc( _error_code );
                           else  throw_xc( "LIBXML2", error_text() );
    }
}

//---------------------------------------------------------------------------Document_ptr::load_xml

void Document_ptr::load_xml( const BSTR xml_text )
{
    load_xml( string_from_bstr( xml_text ) );
}

//--------------------------------------------------------Document_ptr::validate_against_dtd_string

void Document_ptr::validate_against_dtd_string( const string& dtd_string ) const
{
    assert( _ptr );

    Dtd_ptr dtd ( dtd_string );
    validate_against_dtd( dtd );
}

//---------------------------------------------------------------Document_ptr::validate_against_dtd

void Document_ptr::validate_against_dtd( xmlDtdPtr dtd ) const
{
    assert( _ptr );

    Print_context print_context;
    xmlValidCtxt  validation_context;   memset( &validation_context, 0, sizeof validation_context );
    

    validation_context.userData = (void*)                  &print_context;
    validation_context.error    = (xmlValidityErrorFunc)   Print_context::static_print;
    validation_context.warning  = (xmlValidityWarningFunc) Print_context::static_print;

    int ok = xmlValidateDtd( &validation_context, (xmlDocPtr)_ptr, dtd );
    if( !ok )  throw_xc( "LIBXML2-004", print_context._messages );
}

//-----------------------------------------------------Document_ptr::validate_against_schema_string
/*
void Document_ptr::validate_against_schema_string( const string& schema_string ) const
{
    Schema_ptr schema ( schema_string );
    validate_against_schema( schema );
}

//------------------------------------------------------------Document_ptr::validate_against_schema

void Document_ptr::validate_against_schema( xmlSchemaPtr schema ) const
{
    Print_context print_context;




    string schema_str;
    xmlSchemaParserCtxtPtr  schema_parser = xmlSchemaNewMemParserCtxt( schema_str, schema_str.length() );



    xmlSchemaParserCtxtPtr parser_context;

     parser = xmlSchemaNewParserCtxt(STR2CSTR(uri));
     sptr = xmlSchemaParse(parser);
     xmlSchemaFreeParserCtxt(parser);
    break;

    xmlValidCtxt  validation_context;   memset( &validation_context, 0, sizeof validation_context );
    

    validation_context.userData = (void*)                  &print_context;
    validation_context.error    = (xmlValidityErrorFunc)   Print_context::static_print;
    validation_context.warning  = (xmlValidityWarningFunc) Print_context::static_print;

    int ok = xmlValidateDtd( &validation_context, (xmlDocPtr)_ptr, dtd );
    if( !ok )  throw_xc( "LIBXML2-004", print_context._messages );
}
*/

//----------------------------------------------------------------------------delete_ascii_encoding

//static char* delete_ascii_encoding( char* xml_text )
//{
//    char*       result         = xml_text;
//    static char ascii_header[] = "<?xml version=\"1.0\" encoding=\"ASCII\"";
//
//    if( strncmp( xml_text, ascii_header, sizeof ascii_header - 1 ) == 0 )
//    {
//        result = xml_text + sizeof ascii_header - 1 - 19;
//        memmove( result, xml_text, 19 );
//    }
//
//    return result;
//}

//--------------------------------------------------------------------------------Document_ptr::xml

string Document_ptr::xml( const string& encoding, const string& indent_string ) const
{ 
    assert( _ptr );

    string   result;
    xmlChar* buffer = NULL;
    int      length = 0;
    char*    xml_text = NULL;


    if( indent_string != "" )  xmlThrDefTreeIndentString( indent_string.c_str() );

    if( encoding != "" )
    {
        xmlDocDumpFormatMemoryEnc( ptr(), &buffer, &length, encoding.c_str(), indent_string == ""? 0 : 1 );
    }
    else
    if( indent_string == ""? 0 : 1 )  xmlDocDumpFormatMemory( ptr(), &buffer, &length, 1 );
                                else  xmlDocDumpMemory      ( ptr(), &buffer, &length );

    if( indent_string != "" )  xmlThrDefTreeIndentString( NULL );

        
    xml_text = (char*)buffer;
    //xml_text = delete_ascii_encoding( (char*)buffer );
    length -= xml_text - (char*)buffer;


    // Doppeltes <?xml?> entfernen (vermutlich ist es doppelt eingefügt worden)
    
    const char* p = (const char*)xml_text;

    if( length > 1+5 )
    {
        const char* p2 = (const char*)memchr( p+1, '<', length - 5 );
        if( p2 &&  memcmp( p2, "<?xml", 5 ) == 0 )  p = p2;
    }


    result = make_string( p, (const char*)xml_text + length - p );

    xmlFree( buffer );


    if( encoding != ""  &&  lcase(encoding) != "utf-8"  &&  result.length() > 5  &&  strnicmp( result.data(), "<?xml", 5 ) != 0 )
    {
        return "<?xml version=\"1.0\" encoding=\"" + encoding + "\"?>" + ( indent_string != ""? "\n" : "" ) + result;
    }

    return result;
}

/*
string Document_ptr::append_child_and_get_xml( const xml::Element_ptr& element )
{
    create();
    appendChild( element );

    return document.xml();
}
*/
//----------------------------------------------------------------------Document_ptr::createElement

Element_ptr Document_ptr::createElement( const string& tagName ) const
{ 
    assert( _ptr );
    return (Element_ptr)createElement( tagName.c_str() ); 
}

//----------------------------------------------------------------------Document_ptr::createElement

Element_ptr Document_ptr::createElement( const char* tagName ) const
{ 
    assert( _ptr );
    return (Element_ptr) xmlNewDocNode( ptr(), NULL, Utf8_string(tagName).utf8(), NULL ); 
}

//---------------------------------------------------------------------Document_ptr::createTextNode

Text_ptr Document_ptr::createTextNode( const string& data ) const
{ 
    assert( _ptr );
    return xmlNewDocText( ptr(), Utf8_string(data).utf8() ); 
}

//----------------------------------------------------------------------Document_ptr::createComment

Comment_ptr Document_ptr::createComment( const string& data ) const            
{ 
    assert( _ptr );
    return createComment( data.c_str() ); 
}

//----------------------------------------------------------------------Document_ptr::createComment

Comment_ptr Document_ptr::createComment( const char*   data ) const
{ 
    assert( _ptr );
    return xmlNewComment( Utf8_string(data).utf8() ); 
}

//-----------------------------------------------------------------Document_ptr::createCDATASection

CDATASection_ptr Document_ptr::createCDATASection( const string& data ) const            
{ 
    assert( _ptr );
    Utf8_string u = data; 
    return xmlNewCDataBlock( ptr(), u.utf8(), u.byte_count() ); 
}

//--------------------------------------------------------Document_ptr::createProcessingInstruction

ProcessingInstruction_ptr Document_ptr::createProcessingInstruction( const string& target, const string& data ) const      
{ 
    assert( _ptr );
    return xmlNewPI( Utf8_string(target).utf8(), Utf8_string(data).utf8() ); 
}

//--------------------------------------------------------------------Document_ptr::documentElement

Element_ptr Document_ptr::documentElement() const                                
{ 
    assert( _ptr );
    return (Element_ptr)xmlDocGetRootElement( ptr() ); 
}

//----------------------------------------------------------------Document_ptr::create_root_element

Element_ptr Document_ptr::create_root_element( const string& name )
{ 
    return appendChild( createElement( name ) ); 
}

//------------------------------------------------------------------------------Document_ptr::clone

Simple_node_ptr Document_ptr::clone( const Simple_node_ptr& node, int extended ) const
{ 
    assert( _ptr );
    xmlNodePtr result = xmlDocCopyNode( node.ptr(), ptr(), extended );

    if( !result )  throw_xc( "LIBXML2-001", "xmlDocCopyNode" );

    return result;
}

//----------------------------------------------------------------------------Simple_node_ptr::free

void Simple_node_ptr::free()
{
    if( _ptr )
    {
        xmlFree( _ptr );
        _ptr = NULL;
    }
}

//--------------------------------------------------------------------------Simple_node_ptr::assign

void Simple_node_ptr::assign( _xmlNode* ptr, NodeType type )
{ 
    _ptr = ptr; 

    if( _ptr )
    {
        try
        {
            assert_type( type ); 
        }
        catch( const exception& )
        {
            _ptr = NULL;
            throw;
        }
    }
}

//---------------------------------------------------------------------Simple_node_ptr::assert_type

void Simple_node_ptr::assert_type( NodeType type )
{ 
    if( !is_type( type ) )  throw_node_type( type ); 
}

//--------------------------------------------------------------------Simple_node_ptr::set_nodeName

void Simple_node_ptr::set_nodeName( const string& name ) const
{ 
    assert( _ptr );
    xmlNodeSetName( _ptr, (xmlChar*)name.c_str() ); 
}

//------------------------------------------------------------------------Simple_node_ptr::nodeName

string Simple_node_ptr::nodeName() const
{ 
    assert( _ptr );
    return sd( _ptr->name ); 
}

//---------------------------------------------------------------------Simple_node_ptr::nodeName_is

bool Simple_node_ptr::nodeName_is( const char* name ) const
{ 
    assert( _ptr );
    return strcmp( (const char*)_ptr->name, name ) == 0; 
}

//-----------------------------------------------------------------------Simple_node_ptr::nodeValue

string Simple_node_ptr::nodeValue() const
{ 
    assert( _ptr );
    return sd_free( xmlNodeGetContent( _ptr ) ); 
}

//------------------------------------------------------------------------Simple_node_ptr::nodeType

NodeType Simple_node_ptr::nodeType() const
{ 
    assert( _ptr );
    return (NodeType)_ptr->type; 
}

//----------------------------------------------------------------------Simple_node_ptr::parentNode

Simple_node_ptr Simple_node_ptr::parentNode() const
{ 
    assert( _ptr );
    return _ptr->parent; 
}

//----------------------------------------------------------------------Simple_node_ptr::childNodes
/*
NodeList_ptr Simple_node_ptr::childNodes() const
{
}
*/
//----------------------------------------------------------------------Simple_node_ptr::firstChild

Simple_node_ptr Simple_node_ptr::firstChild() const
{ 
    assert( _ptr );
    return _ptr->children; 
}

//-----------------------------------------------------------------------Simple_node_ptr::lastChild

Simple_node_ptr Simple_node_ptr::lastChild() const
{ 
    assert( _ptr );
    return xmlGetLastChild( _ptr ); 
}

//-----------------------------------------------------------------Simple_node_ptr::previousSibling

Simple_node_ptr Simple_node_ptr::previousSibling() const
{ 
    assert( _ptr );
    return _ptr->prev; 
}

//---------------------------------------------------------------------Simple_node_ptr::nextSibling

Simple_node_ptr Simple_node_ptr::nextSibling() const
{ 
    assert( _ptr );
    return _ptr->next; 
}

//--------------------------------------------------------------------Simple_node_ptr::replaceChild

Simple_node_ptr Simple_node_ptr::replaceChild( const Simple_node_ptr& newChild, const Simple_node_ptr& oldChild ) const 
{ 
    assert( _ptr );
    return xmlReplaceNode( oldChild, newChild ); 
}

//--------------------------------------------------------------------Simple_node_ptr::replace_with

Simple_node_ptr Simple_node_ptr::replace_with( const Simple_node_ptr& node )
{
    assert( _ptr );
    return parentNode().replaceChild( node, *this );
}

//---------------------------------------------------------------------Simple_node_ptr::removeChild

void Simple_node_ptr::removeChild( const Simple_node_ptr& child ) const 
{ 
    assert( _ptr );
    xmlUnlinkNode( child ); 
    xmlFree( child );
}

//--------------------------------------------------------------------Simple_node_ptr::insertBefore

Simple_node_ptr Simple_node_ptr::insertBefore( const Simple_node_ptr& newChild, const Simple_node_ptr& refChild ) const
{
    assert( _ptr );
    if( refChild )
    {
        return xmlAddPrevSibling( refChild, newChild );      // "merging adjacent TEXT nodes (@newChild may be freed)"
    }
    else
        return appendChild( newChild );
}

//---------------------------------------------------------------------Simple_node_ptr::appendChild

Simple_node_ptr Simple_node_ptr::appendChild( const Simple_node_ptr& newChild ) const
{ 
    assert( _ptr );
    return xmlAddChild( _ptr, newChild ); 
}

//-------------------------------------------------------------------Simple_node_ptr::hasChildNodes

bool Simple_node_ptr::hasChildNodes() const
{ 
    assert( _ptr );
    return _ptr->children != NULL; 
}

//------------------------------------------------------------------Simple_node_ptr::getTextContent

string Simple_node_ptr::getTextContent() const
{ 
    assert( _ptr );
    return sd_free( xmlNodeGetContent( _ptr ) ); 
}

//---------------------------------------------------------------------Simple_node_ptr::line_number

int Simple_node_ptr::line_number() const
{ 
    assert( _ptr );
    return XML_GET_LINE( _ptr ); 
}

//-----------------------------------------------------------------Simple_node_ptr::throw_node_type    

void Simple_node_ptr::throw_node_type( NodeType type ) const
{
    Xc x ( "LIBXML2-002" );

    x.insert( 1, name_of_node_type( type ) );
    x.insert( 2, name_of_node_type( nodeType() ) );
    x.insert( 3, nodeName() );

    x.append_text( getTextContent().substr( 0, 100 ) );

    throw_xc( x );
}

//-------------------------------------------------------------------Simple_node_ptr::contains_node
// Nicht getestet und wird nicht benutzt.

static bool node_contains_node(const xmlNode*, const xmlNode*);

static bool list_contains_node(const xmlNode* head, const xmlNode* node) {
    for (const xmlNode* c = head; c; c = c->next) {
        if (c == node)  return true;
        if (node_contains_node(c, node))  return true;
    }
    return false;
}

static bool node_contains_node(const xmlNode* a, const xmlNode* b) {
    if (list_contains_node((const xmlNode*)a->properties, b))  return true;
    if (list_contains_node(a->children, b))  return true;
    return false;
}

bool Simple_node_ptr::contains_node(const Simple_node_ptr& node) const {
    return node_contains_node(_ptr, node._ptr);
}

//----------------------------------------------------------------------Simple_node_ptr::childNodes

NodeList_ptr Simple_node_ptr::childNodes() const
{ 
    assert( _ptr );
    return _ptr->children; 
}

//-------------------------------------------------------------------Simple_node_ptr::ownerDocument

Document_ptr Simple_node_ptr::ownerDocument() const
{ 
    assert( _ptr );
    return _ptr->doc; 
}

//-----------------------------------------------------------------------Simple_node_ptr::cloneNode

Simple_node_ptr Simple_node_ptr::cloneNode( bool deep ) const
{ 
    assert( _ptr );

    xmlNodePtr result = xmlCopyNode( _ptr, deep? 1 : 2 ); 
    if( !result )  throw_xc( "xmlCopyNode" );

    return result;
}

//--------------------------------------------------------------Simple_node_ptr::xml_without_prolog

string Simple_node_ptr::xml_without_prolog( const string& encoding, bool indented ) const
{
    assert( _ptr );

    string result = xml( encoding, indented );
    
    if( string_begins_with( result, "<?" ) )
    {
        size_t pos = result.find( "?>" );
        if( pos != string::npos ) 
        {
            pos += 2;
            while( pos < result.length()  &&  isspace( (unsigned char)result[ pos ] ) )  pos++;
            result.erase( 0, pos );
        }
    }

    return result;
}

//-----------------------------------------------------------------------------Simple_node_ptr::xml

string Simple_node_ptr::xml( const string& encoding, bool indented ) const
{ 
    assert( _ptr );

    string result;

    if( encoding == "" )
    {
        xmlBufferPtr buffer = xmlBufferCreate();

        if( !buffer )  throw_xc( "LIBXML2-001", "xmlBufferCreate" );

        int length = xmlNodeDump( buffer, ownerDocument().ptr(), _ptr, 0, indented? 1 : 0 );
        
        if( length > 0 )  result = sd( buffer->content );
        xmlBufferFree( buffer );

        if( length < 0 )  throw_xc( "LIBXML2-001", "xmlNodeDump" );
    }
    else
    {
        xmlCharEncodingHandler* encoding_handler = xmlFindCharEncodingHandler( encoding.c_str() );
        if( !encoding_handler )  throw_xc( "LIBXML2-001", "xmlFindCharEncodingHandler", encoding );

        xmlOutputBufferPtr output_buffer = xmlAllocOutputBuffer( encoding_handler );
        if( !output_buffer )  throw_xc( "LIBXML2-001", "xmlAllocOutputBuffer" );

        xmlNodeDumpOutput( output_buffer, ownerDocument().ptr(), _ptr, 0, indented? 1 : 0, encoding.c_str() );
        xmlOutputBufferFlush( output_buffer );
        
        char* xml_text = (char*)( output_buffer->conv? output_buffer->conv->content : output_buffer->buffer->content );
        result = xml_text;
        //result = delete_ascii_encoding( xml_text );

        xmlOutputBufferClose( output_buffer );
    }

    if( encoding != ""  &&  lcase(encoding) != "utf-8"  &&  result.length() > 5  &&  strnicmp( result.data(), "<?xml", 5 ) != 0 )
    {
        return "<?xml version=\"1.0\" encoding=\"" + encoding + "\"?>" + ( indented? "\n" : "" ) + result;
    }

    return result;
}

//--------------------------------------------------------------------Simple_node_ptr::select_nodes

Xpath_nodes Simple_node_ptr::select_nodes( const string& xpath_expression ) const
{
    Xpath_nodes result;
    
    if( _ptr )
    {
        result._xpath_object = Z_NEW( Xpath_nodes::Xpath_object );

        result._xpath_object->_xpath_context = xmlXPathNewContext( _ptr->doc );
        if( !result._xpath_object->_xpath_context )  throw_xc( "LIBXML2-001", "xmlXPathNewContext" );

        result._xpath_object->_xpath_context->node = _ptr;

        result._xpath_object->_xpath_object = xmlXPathEvalExpression( Utf8_string( xpath_expression ).utf8(), result._xpath_object->_xpath_context );
        if( !result._xpath_object->_xpath_object )  throw_xc( "LIBXML2-001", "xmlXPathEvalExpression", xpath_expression );
    }

    return result;
}

//---------------------------------------------------------------------Simple_node_ptr::select_node

Simple_node_ptr Simple_node_ptr::select_node( const string& xpath_expression ) const
{
    Xpath_nodes nodes = select_nodes( xpath_expression );
    return nodes.count() > 0? nodes[0] : NULL;
}

//------------------------------------------------------------------------Simple_node_ptr::has_node

bool Simple_node_ptr::has_node( const string& xpath_expression ) const
{
    return select_node( xpath_expression ) != NULL;
}

//--------------------------------------------------------------Simple_node_ptr::select_node_strict

Simple_node_ptr Simple_node_ptr::select_node_strict( const string& xpath_expression ) const
{
    Simple_node_ptr result = select_node( xpath_expression );
    if( !result )  throw_xc( "LIBXML2-008", xpath_expression );
    return result;
}

//-----------------------------------------------------------Simple_node_ptr::select_element_strict

Element_ptr Simple_node_ptr::select_element_strict( const string& xpath_expression ) const
{
    return Element_ptr( select_node_strict( xpath_expression ) );
}

//-------------------------------------------------------------------------Node_ptr::is_in_document

bool Node_ptr::is_orphan() const {
    if (!_ptr)  return false;
    if (!_document)  return true;   // Sollte das passieren?
    return !_document.contains_node(*this);
}

//---------------------------------------------------------------------------Node_ptr::set_document

void Node_ptr::set_document()
{ 
    _document = _ptr? _ptr->doc : NULL; 
}

//---------------------------------------------------------Xpath_nodes::Xpath_object::~Xpath_object

Xpath_nodes::Xpath_object::~Xpath_object()
{
    xmlXPathFreeObject( _xpath_object );
    xmlXPathFreeContext( _xpath_context ); 
}

//-------------------------------------------------------------------------------Xpath_nodes::count

int Xpath_nodes::count() const
{
    return _xpath_object &&  _xpath_object->_xpath_object->nodesetval? _xpath_object->_xpath_object->nodesetval->nodeNr : 0;
}

//---------------------------------------------------------------------------------Xpath_nodes::get

Simple_node_ptr Xpath_nodes::get( int i ) const
{
    Simple_node_ptr result = _xpath_object->_xpath_object->nodesetval->nodeTab[ i ];

    assert( result.nodeType() == ELEMENT_NODE );

    return result;
}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const string& name, const char* value ) const   
{ 
    assert( _ptr );
    xmlSetProp( _ptr, Utf8_string(name).utf8(), Utf8_string(value).utf8() ); 
}

//------------------------------------------------------------------------Element_ptr::getAttribute

string Element_ptr::getAttribute( const string& name, const string& deflt ) const  
{ 
    assert( _ptr );
    return getAttribute( name.c_str(), deflt.c_str() ); 
}

//--------------------------------------------------------------------Element_ptr::getAttributeNode

Attr_ptr Element_ptr::getAttributeNode( const char* name ) const
{ 
    assert( _ptr );

    Attr_ptr attr = xmlHasProp( _ptr, Utf8_string(name).utf8() ); 

    // ACHTUNG:
    // Bei DOCTYPE: Fehlendes Attribut wird als "" geliefert, wenn es als #IMPLIED deklariert ist.
    // ATTRIBUT="" WIRKT WIE EIN NICHT ANGEGEBENES ATTRIBUT!

    if( attr  &&  attr.value() == "" )  attr = NULL;        

    return attr;
}

//-----------------------------------------------------------------------Element_ptr::hasAttributes

bool Element_ptr::hasAttributes() const
{
    assert( _ptr );
    return ptr()->attributes != NULL;
}

//------------------------------------------------------------------------Element_ptr::getAttribute

string Element_ptr::getAttribute( const char* name, const char* deflt ) const  
{ 
    assert( _ptr );

    Attr_ptr a = getAttributeNode( name ); 
    return a? a.value() : deflt; 
}

//--------------------------------------------------------------Element_ptr::getAttribute_mandatory

string Element_ptr::getAttribute_mandatory( const string& name ) const  
{
    assert( _ptr );

    string result = getAttribute( name );
    if( result == ""  )  throw_xc( "LIBXML2-003", name, nodeName() );
    return result;
}

//-------------------------------------------------------------------Element_ptr::bool_getAttribute

bool Element_ptr::bool_getAttribute( const string& name, bool deflt ) const
{
    assert( _ptr );

    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_bool( attr.value() );
          else  return deflt;
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::int_getAttribute( const string& name ) const
{
    assert( _ptr );

    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_int( attr.value() );
          else  throw_xc( "LIBXML2-003", name, nodeName() );
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::int_getAttribute( const string& name, int deflt ) const
{
    assert( _ptr );

    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_int( attr.value() );
          else  return deflt;
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::uint_getAttribute( const string& name, int deflt ) const
{
    assert( _ptr );

    Attr_ptr attr = getAttributeNode( name );
    if( attr )  return as_uint( attr.value() );
          else  return deflt;
}

//-----------------------------------------------------------------Element_ptr::time_t_getAttribute

//time_t Element_ptr::time_t_getAttribute( const string& name, time_t deflt ) const
//{
//    Attr_ptr attr = getAttributeNode( name );
//    if( attr )  return as_uint( attr.value() );
//          else  return deflt;
//}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const string& name, bool value ) const
{
    setAttribute( name, as_string( value ) );
}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const string& name, int64 value ) const
{
    setAttribute( name, as_string( value ) );
}

//------------------------------------------------------------------------Element_ptr::hasAttribute

bool Element_ptr::hasAttribute( const string& name ) const
{
    assert( _ptr );

    return xmlHasProp( _ptr, Utf8_string(name).utf8() ) != NULL;
}

//------------------------------------------------------------------------Element_ptr::hasAttribute

bool Element_ptr::removeAttribute( const string& name ) const
{
    assert( _ptr );

    bool result = false;

    if( xmlAttrPtr attribute_ptr = xmlHasProp( _ptr, Utf8_string(name).utf8() ) )
    {
        int error = xmlRemoveProp( attribute_ptr );
        if( error )  throw_xc( "LIBXML2-001", "xmlRemoveProp", name );
        result = true;
    }

    return result;
}

//-----------------------------------------------------------------Element_ptr::first_child_element

Element_ptr Element_ptr::first_child_element() const
{
    Simple_node_ptr node = firstChild();

    while( node  &&  node.nodeType() != ELEMENT_NODE )  node = node.nextSibling();

    return Element_ptr( node );
}

//------------------------------------------------------------------Element_ptr::append_new_element

Element_ptr Element_ptr::append_new_element( const string& element_name ) const
{
    Element_ptr element = ownerDocument().createElement( element_name );
    appendChild( element );
    return element;
}

//-------------------------------------------------------------Element_ptr::append_new_text_element

Element_ptr Element_ptr::append_new_text_element( const string& element_name, const string& text ) const
{
    Element_ptr element = append_new_element( element_name );
    element.appendChild( ownerDocument().createTextNode( text ) );
    return element;
}

//-------------------------------------------------------------Element_ptr::append_new_text_element

Element_ptr Element_ptr::append_new_cdata_element( const string& element_name, const string& text ) const
{
    Element_ptr element = append_new_element( element_name );
    element.appendChild( ownerDocument().createCDATASection( text ) );
    return element;
}

//----------------------------------------------------Element_ptr::append_new_cdata_or_text_element

Element_ptr Element_ptr::append_new_cdata_or_text_element( const string& element_name, const string& text ) const
{
    if( text.find( "]]>" ) != string::npos  ||  
        text.find( '<' ) == string::npos && 
        text.find( '&' ) == string::npos )
    {
        return append_new_text_element( element_name, text );
    }
    else
    {
        return append_new_cdata_element( element_name, text );
    }
}

//----------------------------------------------Element_ptr::append_new_name_value_element_optional

Element_ptr Element_ptr::append_new_name_value_element_optional( const string& element_name, 
                                                                 const string& name_attribute_name, const string& name_value, 
                                                                 const string& value_attribute_name, const string& value_value )
{
    return append_new_name_value_element_optional( element_name.c_str(), 
                                                   name_attribute_name.c_str(), name_value.c_str(), 
                                                   value_attribute_name.c_str(), value_value );
}

//----------------------------------------------Element_ptr::append_new_name_value_element_optional

Element_ptr Element_ptr::append_new_name_value_element_optional( const char* element_name, 
                                                                 const char* name_attribute_name, const char* name_value, 
                                                                 const char* value_attribute_name, const string& value_value )
{
    Element_ptr result;

    if( value_value != "" )
    {
        result = append_new_element( element_name );
        result.setAttribute( name_attribute_name , name_value  );
        result.setAttribute( value_attribute_name, value_value );
    }

    return result;
}

//------------------------------------------------------------------Element_ptr::append_new_comment

Comment_ptr Element_ptr::append_new_comment( const string& comment )
{
    Comment_ptr result = ownerDocument().createComment( comment );
    appendChild( result );
    return result;
}

//---------------------------------------------------------------------------------Element_ptr::xml
/*
string Element_ptr::xml( bool indented ) const
{ 
    string   result;
    xmlChar* buffer = NULL;
    int      length = 0;

    if( indented )  xmlNodeDump( ptr(), &buffer, &length, 1 );
              else  xmlElemDump( ptr(), &buffer, &length );


    // Doppeltes <?xml?> entfernen
    
    const char* p = (const char*)buffer;

    if( length > 1+5 )
    {
        const char* p2 = (const char*)memchr( p+1, '<', min( 400, length - 5 ) );
        if( p2 &&  memcmp( p2, "<?xml", 5 ) == 0 )  p = p2;
    }


    result = make_string( p, (const char*)buffer + length - p );

    xmlFree( buffer );

    return result;
}
*/
//-------------------------------------------------------------------------------Document_ptr::text

string Element_ptr::text() const
{
    if( !this  ||  !_ptr )  return "";


    String_list result_list;

    for( Simple_node_ptr node = firstChild(); node; node = node.nextSibling() )
    {
        switch( node.nodeType() )
        {
            case TEXT_NODE: 
                result_list.append( Text_ptr(node).data() );
                break;

            case CDATA_SECTION_NODE: 
                result_list.append( Text_ptr(node).data() );
                break;

            case ELEMENT_NODE:
                result_list.append( Element_ptr(node).text() );

            default: ;
        }
    }

    return result_list.to_string();
}

//-------------------------------------------------------------------------------Document_ptr::text

string Element_ptr::trimmed_text() const
{
    string text = this->text();

    const char* p0 = text.c_str();
    const char* p  = p0;
    while( *p == ' ' || *p == '\t' )  p++;            // Leerzeichen hinter <tag> löschen
    if( *p == '\r' )  p++;              // Ersten Zeilenwechsel löschen
    if( *p == '\n' )  p++;

    const char* p_end = p0 + text.length();
    while( p_end > p0  &&  ( p_end[-1] == ' ' || p_end[-1] == '\t' ) )  p_end--;    // Leerzeichen vor </tag> löschen
    if( p_end > p0  &&  p_end[-1] == '\n' )  p_end--;                           // Letzten Zeilenwechsel löschen
    if( p_end > p0  &&  p_end[-1] == '\r' )  p_end--;

    //while( p_end > p  &&  isspace( (uchar)p_end[-1] ) )  p_end--;        // Alle Leerzeichen und Blanks etc. am Ende löshcen

    text.erase( p_end - p0 );
    text.erase( 0, p - p0 );

    return text;
}

//--------------------------------------------------------------------------------Comment_ptr::data

string Comment_ptr::data() const
{ 
    assert( _ptr );

    return sd_free( xmlNodeGetContent( _ptr ) ); 
}

//-----------------------------------------------------------------------------------Text_ptr::data

string Text_ptr::data() const
{ 
    assert( _ptr );

    return sd_free( xmlNodeGetContent( _ptr ) ); 
}

//----------------------------------------------------------------------------------Attr_ptr::value

string Attr_ptr::value()
{ 
    assert( _ptr );

    return sd_free( xmlNodeGetContent( _ptr->children ) ); 
}

//---------------------------------------------------------------------------------Dtd_ptr::release

void Dtd_ptr::release()
{ 
    if( _ptr )
    {
        xmlFreeDtd( _ptr );
        _ptr = NULL; 
    }
}

//------------------------------------------------------------------------------------Dtd_ptr::read

void Dtd_ptr::read( const string& dtd_string )
{
    release();

    /*
    XML_CHAR_ENCODING_8859_1, XML_CHAR_ENCODING_UTF8 
    xmlIOParseDTD        (xmlSAXHandlerPtr sax,                      xmlParserInputBufferPtr input,                      xmlCharEncoding enc)

    xmlParserCtxtPtr xmlCreateIOParserCtxt    (xmlSAXHandlerPtr sax,
                     void *user_data,
                     xmlInputReadCallback   ioread,
                     xmlInputCloseCallback  ioclose,
                     void *ioctx,
                     xmlCharEncoding enc);

    */

    xmlParserInputBufferPtr input_buffer = xmlParserInputBufferCreateMem( dtd_string.data(), dtd_string.length(), XML_CHAR_ENCODING_NONE ); 
    if( !input_buffer )  throw_xc( "LIBXML2-001", "xmlParserInputBufferCreateMem()" );


    /*
    xmlSAXHandler sax_handler;
    memset( &sax_handler, 0, sizeof sax_handler );

    sax_handler.fatalError  = (fatalErrorSAXFunc)sax_print;
    sax_handler.error       = (errorSAXFunc)     sax_print;
    sax_handler.warning     = (warningSAXFunc)   sax_print;
    sax_handler.initialized = 1;
    */

    //Z_MUTEX( libxml_parser_mutex )
    {
        //static_parser_messages = "";

        _ptr = xmlIOParseDTD( NULL, input_buffer, XML_CHAR_ENCODING_NONE ); 

        if( !_ptr )  throw_xc( "LIBXML2-001", "xmlIOParseDTD()", static_parser_messages );
    }
    //Offenbar von xmlIOParseDTD() erledigt: xmlFreeParserInputBuffer( input_buffer );

}

//------------------------------------------------------------------------------Schema_ptr::release

void Schema_ptr::release()
{ 
    if( _ptr )
    {
        xmlSchemaFree( _ptr );
        _ptr = NULL; 
    }
}

//---------------------------------------------------------------------------------Schema_ptr::read
/*
void Schema_ptr::read( const string& schema_string )
{
    release();

    xmlSchemaParserCtxtPtr parser_context = xmlSchemaNewMemParserCtx( schema_string.data(), schema_string.length() );
    if( !parser_context )  throw_xc( "LIBXML2-001", "xmlSchemaNewMemParserCtx()", static_parser_messages );

    _ptr = xmlSchemaParse( parser_context );
    xmlSchemaFreeParserCtxt( parser_context );

    if( !_ptr )  throw_xc( "LIBXML2-001", "xmlSchemaParse()", static_parser_messages );
}
*/
//---------------------------------------------------------------------------------Schema_ptr::read

void Schema_ptr::read( const Document_ptr& schema_document )
{
    release();

    xmlSchemaParserCtxtPtr parser_context = xmlSchemaNewDocParserCtxt( schema_document.ptr() );
    if( !parser_context )  throw_xc( "LIBXML2-001", "xmlSchemaNewDocParserCtxt()", static_parser_messages );

    _ptr = xmlSchemaParse( parser_context );
    xmlSchemaFreeParserCtxt( parser_context );

    if( !_ptr )  throw_xc( "LIBXML2-001", "xmlSchemaParse()", static_parser_messages );
}

//-----------------------------------------------------------------------------Schema_ptr::validate

void Schema_ptr::validate( const Document_ptr& document )
{
    if( !_ptr )  throw_xc( "LIBXML2-006", "validate()" );

    Print_context print_context;

    xmlSchemaValidCtxt* validation_context = xmlSchemaNewValidCtxt( _ptr );
    if( !validation_context )  throw_xc( "LIBXML2-001", "xmlSchemaNewValidCtxt()", static_parser_messages );

    xmlSchemaSetValidErrors( validation_context, Print_context::static_print, Print_context::static_print, &print_context );

    Z_LOG2("scheduler.xml", "xmlSchemaValidateDoc()\n" );
    int error = xmlSchemaValidateDoc( validation_context, document.ptr() );
    xmlSchemaFreeValidCtxt( validation_context );

    if( error )  throw_xc( "LIBXML2-007", print_context._messages );
}

//-------------------------------------------------------------------------------------------------

} //namespace libxml2
} //namespace xml
} //namespace zschimmer

//-------------------------------------------------------------------------------------------------
