#include "zschimmer.h"
#if defined Z_USE_JAVAXML

#include "xml_java.h"
#include "mutex.h"
#include "log.h"
#include "string_list.h"
#include <stdarg.h>

#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__util__XmlUtils.h"
typedef ::javaproxy::com::sos::scheduler::engine::kernel::util::XmlUtils XmlUtilsJ;

using namespace javaproxy::org::w3c::dom;
using namespace std;

namespace zschimmer {
namespace xml {

#if !defined Z_WINDOWS
    const int NODE_ELEMENT = 1;
    const int NODE_ATTRIBUTE = 2;
    const int NODE_TEXT	= 3;
    const int NODE_CDATA_SECTION = 4;
    const int NODE_ENTITY_REFERENCE = 5;
    const int NODE_ENTITY = 6;
    const int NODE_PROCESSING_INSTRUCTION = 7;
    const int NODE_COMMENT = 8;
    const int NODE_DOCUMENT = 9;
    const int NODE_DOCUMENT_TYPE = 10;
    const int NODE_DOCUMENT_FRAGMENT = 11;
    const int NODE_NOTATION = 12;
#endif	

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

//-----------------------------------------------------------------------------Document_ptr::create

Document_ptr& Document_ptr::create() { 
    assign(XmlUtilsJ::newDocument());
    return *this;
}

//-----------------------------------------------------------------------------Document_ptr::create

void Document_ptr::assign(const DocumentJ& doc) { 
    _documentJ = doc;
    Simple_node_ptr::assign(NodeJ(doc));
}

//-----------------------------------------------------------------------Document_ptr::try_load_xml

bool Document_ptr::try_load_xml( const BSTR xml_text_bstr ) {
    return try_load_xml( com::string_from_bstr( xml_text_bstr ) );
}

//-----------------------------------------------------------------------Document_ptr::try_load_xml

bool Document_ptr::try_load_xml( const string& xml_text, const string& encoding ) {
    try {
        load_xml(xml_text);
        return true;
    } catch (exception&x) {
        Z_LOG("try_load_xml() ==> " << x.what() << "\n");
        return false;
    }
}

//---------------------------------------------------------------------------Document_ptr::load_xml

void Document_ptr::load_xml(const string& xml_text, const string& encoding) 
{
    assign(XmlUtilsJ::loadXml(javabridge::Local_java_byte_array(xml_text)));        //TOOD encoding berücksichtigen
}

//---------------------------------------------------------------------------Document_ptr::load_xml

void Document_ptr::load_xml( const BSTR xml_text )
{
    load_xml( com::string_from_bstr( xml_text ) );
}

//--------------------------------------------------------------------------------Document_ptr::xml

string Document_ptr::xml( const string& encoding, const string& indent_string ) const
{ 
    return XmlUtilsJ::toXmlBytes(Simple_node_ptr::ref(), encoding, !indent_string.empty());
}

//----------------------------------------------------------------------Document_ptr::createElement

Element_ptr Document_ptr::createElement( const string& tagName ) const
{ 
    return Element_ptr(_documentJ.createElement(tagName));
}

//---------------------------------------------------------------------Document_ptr::createTextNode

Text_ptr Document_ptr::createTextNode( const string& data ) const
{ 
    return Text_ptr(_documentJ.createTextNode(data));
}

//----------------------------------------------------------------------Document_ptr::createComment

Comment_ptr Document_ptr::createComment( const string& data ) const            
{ 
    return Comment_ptr(_documentJ.createComment(data));
}

//-----------------------------------------------------------------Document_ptr::createCDATASection

CDATASection_ptr Document_ptr::createCDATASection( const string& data ) const            
{ 
    return CDATASection_ptr(_documentJ.createCDATASection(data));
}

//--------------------------------------------------------Document_ptr::createProcessingInstruction
/*
ProcessingInstruction_ptr Document_ptr::createProcessingInstruction( const string& target, const string& data ) const      
{ 
    return ProcessingInstruction_ptr(_documentJ.create...);
    return xmlNewPI( Utf8_string(target).utf8(), Utf8_string(data).utf8() ); 
}
*/
//--------------------------------------------------------------------Document_ptr::documentElement

Element_ptr Document_ptr::documentElement() const                                
{ 
    return Element_ptr(_documentJ.getDocumentElement());
}

//----------------------------------------------------------------Document_ptr::create_root_element

Element_ptr Document_ptr::create_root_element( const string& name )
{ 
    return appendChild( createElement( name ) ); 
}

//-------------------------------------------------------------------------Document_ptr::importNode

Simple_node_ptr Document_ptr::importNode(const Simple_node_ptr& node) const 
{
    bool deep = true;
    return _documentJ.importNode(node.ref(), deep);
}

//--------------------------------------------------------------------------Simple_node_ptr::assign

void Simple_node_ptr::assign(const NodeJ& nodeJ, NodeType type )
{ 
    _nodeJ = nodeJ; 
    if(_nodeJ) {
        try {
            assert_type( type ); 
        }
        catch( const exception& ) {
            _nodeJ = NULL;
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
    _nodeJ.setNodeValue(name);
}

//------------------------------------------------------------------------Simple_node_ptr::nodeName

string Simple_node_ptr::nodeName() const
{ 
    return _nodeJ.getNodeName();
}

//---------------------------------------------------------------------Simple_node_ptr::nodeName_is

bool Simple_node_ptr::nodeName_is( const char* name ) const
{ 
    return nodeName() == name;
}

//-----------------------------------------------------------------------Simple_node_ptr::nodeValue

string Simple_node_ptr::nodeValue() const
{ 
    return _nodeJ.getNodeValue();
}

//------------------------------------------------------------------------Simple_node_ptr::nodeType

NodeType Simple_node_ptr::nodeType() const
{ 
    return (NodeType)_nodeJ.getNodeType(); 
}

//----------------------------------------------------------------------Simple_node_ptr::parentNode

Simple_node_ptr Simple_node_ptr::parentNode() const
{ 
    return Simple_node_ptr(_nodeJ.getParentNode());
}

//----------------------------------------------------------------------Simple_node_ptr::firstChild

Simple_node_ptr Simple_node_ptr::firstChild() const
{ 
    return Simple_node_ptr(_nodeJ.getFirstChild());
}

//-----------------------------------------------------------------------Simple_node_ptr::lastChild

Simple_node_ptr Simple_node_ptr::lastChild() const
{ 
    return Simple_node_ptr(_nodeJ.getLastChild());
}

//---------------------------------------------------------------------Simple_node_ptr::nextSibling

Simple_node_ptr Simple_node_ptr::nextSibling() const
{ 
    return Simple_node_ptr(_nodeJ.getNextSibling());
}

//--------------------------------------------------------------------Simple_node_ptr::replaceChild

Simple_node_ptr Simple_node_ptr::replaceChild( const Simple_node_ptr& newChild, const Simple_node_ptr& oldChild ) const 
{ 
    return Simple_node_ptr(_nodeJ.replaceChild(newChild.ref(), oldChild.ref()));
}

//--------------------------------------------------------------------Simple_node_ptr::replace_with

Simple_node_ptr Simple_node_ptr::replace_with( const Simple_node_ptr& node )
{
    return parentNode().replaceChild(node, *this);
}

//---------------------------------------------------------------------Simple_node_ptr::removeChild

void Simple_node_ptr::removeChild( const Simple_node_ptr& child ) const 
{ 
    _nodeJ.removeChild(child.ref());
}

//--------------------------------------------------------------------Simple_node_ptr::insertBefore

Simple_node_ptr Simple_node_ptr::insertBefore( const Simple_node_ptr& newChild, const Simple_node_ptr& refChild ) const
{
     return _nodeJ.insertBefore(newChild.ref(), refChild.ref());
}

//--------------------------------------------------------------Simple_node_ptr::appendForeignChild

Simple_node_ptr Simple_node_ptr::appendForeignChild(const Simple_node_ptr& newChild) const 
{
    bool deep = true;
    NodeJ importedNode = thisOrOwnerDocumentJ().importNode(newChild.ref(), deep);
    return _nodeJ.appendChild(importedNode); 
}

//--------------------------------------------------------------Simple_node_ptr::appendForeignChild

Simple_node_ptr Simple_node_ptr::adoptAndAppendChild(const Simple_node_ptr& newChild) const 
{
    NodeJ adoptedNode = thisOrOwnerDocumentJ().adoptNode(newChild.ref());
    return _nodeJ.appendChild(adoptedNode); 
}

//------------------------------------------------------------Simple_node_ptr::thisOrOwnerDocumentJ

DocumentJ Simple_node_ptr::thisOrOwnerDocumentJ() const {
    if (const Document_ptr* d = dynamic_cast<const Document_ptr*>(this))
        return d->ref();
    else 
        return _nodeJ.getNodeType() == NODE_DOCUMENT? (DocumentJ)_nodeJ : _nodeJ.getOwnerDocument();
}

//---------------------------------------------------------------------Simple_node_ptr::appendChild

Simple_node_ptr Simple_node_ptr::appendChild( const Simple_node_ptr& newChild ) const
{ 
    return _nodeJ.appendChild(newChild.ref()); 
}

//-------------------------------------------------------------------Simple_node_ptr::hasChildNodes

bool Simple_node_ptr::hasChildNodes() const
{ 
    return _nodeJ.hasChildNodes(); 
}

//------------------------------------------------------------------Simple_node_ptr::getTextContent

string Simple_node_ptr::getTextContent() const
{ 
    return _nodeJ.getTextContent();
}

//---------------------------------------------------------------------Simple_node_ptr::line_number

int Simple_node_ptr::line_number() const
{ 
    return 0;
}

//-----------------------------------------------------------------Simple_node_ptr::throw_node_type    

void Simple_node_ptr::throw_node_type( NodeType type ) const
{
    Xc x ( "JAVAXML-002" );
    x.insert( 1, name_of_node_type( type ) );
    x.insert( 2, name_of_node_type( nodeType() ) );
    x.insert( 3, nodeName() );
    x.append_text( getTextContent().substr( 0, 100 ) );
    throw_xc( x );
}

//-------------------------------------------------------------------Simple_node_ptr::ownerDocument

Document_ptr Simple_node_ptr::ownerDocument() const
{ 
    return Document_ptr(_nodeJ.getOwnerDocument());
}

//-----------------------------------------------------------------------Simple_node_ptr::cloneNode

Simple_node_ptr Simple_node_ptr::cloneNode( bool deep ) const
{ 
    return _nodeJ.cloneNode(deep);
}

//--------------------------------------------------------------Simple_node_ptr::xml_without_prolog

string Simple_node_ptr::xml_without_prolog( const string& encoding, bool indented ) const
{
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
    return XmlUtilsJ::toXml(_nodeJ);
    //if( encoding != ""  &&  lcase(encoding) != "utf-8"  &&  result.length() > 5  &&  strnicmp( result.data(), "<?xml", 5 ) != 0 )
    //{
    //    return "<?xml version=\"1.0\" encoding=\"" + encoding + "\"?>" + ( indented? "\n" : "" ) + result;
    //}
    //return result;
}

//--------------------------------------------------------------------Simple_node_ptr::select_nodes

Node_list Simple_node_ptr::select_nodes( const string& xpath_expression ) const
{
    return _nodeJ? NodeList(XmlUtilsJ::xpathNodeList(_nodeJ, xpath_expression)) : NodeList();
}

//---------------------------------------------------------------------Simple_node_ptr::select_node

Simple_node_ptr Simple_node_ptr::select_node( const string& xpath_expression ) const
{
    return _nodeJ? XmlUtilsJ::xpathNode(_nodeJ, xpath_expression) : NULL;
}

//------------------------------------------------------------------------Simple_node_ptr::has_node

bool Simple_node_ptr::has_node( const string& xpath_expression ) const
{
    return select_node( xpath_expression );
}

//--------------------------------------------------------------Simple_node_ptr::select_node_strict

Simple_node_ptr Simple_node_ptr::select_node_strict( const string& xpath_expression ) const
{
    Simple_node_ptr result = select_node( xpath_expression );
    if( !result )  throw_xc( "JAVAXML-008", xpath_expression );
    return result;
}

//-----------------------------------------------------------Simple_node_ptr::select_element_strict

Element_ptr Simple_node_ptr::select_element_strict( const string& xpath_expression ) const
{
    return Element_ptr( select_node_strict( xpath_expression ) );
}

//-------------------------------------------------------------------------------Node_list::count

int Node_list::count() const
{
    return _nodeListJ? _nodeListJ.getLength() : 0;
}

//---------------------------------------------------------------------------------Node_list::get

Simple_node_ptr Node_list::get( int i ) const
{
    return _nodeListJ.item(i);
}

//------------------------------------------------------------------------Element_ptr::setAttribute

void Element_ptr::setAttribute( const string& name, const char* value ) const   
{ 
    _elementJ.setAttribute(name, value);
}

//------------------------------------------------------------------------Element_ptr::getAttribute

string Element_ptr::getAttribute( const char* name, const char* deflt ) const  
{ 
    return getAttribute(string(name), string(deflt));
}

//------------------------------------------------------------------------Element_ptr::getAttribute

string Element_ptr::getAttribute( const string& name, const string& deflt ) const  
{ 
    return name == "" || _elementJ.hasAttribute(name)? (string)_elementJ.getAttribute(name) : deflt;
}

//-----------------------------------------------------------------------Element_ptr::hasAttributes

bool Element_ptr::hasAttributes() const
{
    return NodeJ(_elementJ).hasAttributes();
}

//--------------------------------------------------------------Element_ptr::getAttribute_mandatory

string Element_ptr::getAttribute_mandatory( const string& name ) const  
{
    string result = getAttribute( name );
    if( result == ""  )  throw_xc( "JAVAXML-003", name, nodeName() );
    return result;
}

//-------------------------------------------------------------------Element_ptr::bool_getAttribute

bool Element_ptr::bool_getAttribute( const string& name, bool deflt ) const
{
    return _elementJ.hasAttribute(name)? as_bool(getAttribute(name)) : deflt;
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::int_getAttribute( const string& name ) const
{
    if (!_elementJ.hasAttribute(name)) throw_xc("JAVAXML-003", name, nodeName());
    return as_int(getAttribute(name));
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::int_getAttribute( const string& name, int deflt ) const
{
    return _elementJ.hasAttribute(name)? as_int(getAttribute(name)) : deflt;
}

//--------------------------------------------------------------------Element_ptr::int_getAttribute

int Element_ptr::uint_getAttribute( const string& name, int deflt ) const
{
    return _elementJ.hasAttribute(name)? as_uint(getAttribute(name)) : deflt;
}

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
    return _elementJ.hasAttribute(name);
}

//------------------------------------------------------------------------Element_ptr::hasAttribute

void Element_ptr::removeAttribute( const string& name ) const
{
    _elementJ.removeAttribute(name);
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

//-------------------------------------------------------------------------------Document_ptr::text

string Element_ptr::text() const
{
    if (!this || !_elementJ )  
        return "";
    else {
        String_list result_list;

        for (Simple_node_ptr node = firstChild(); node; node = node.nextSibling()) {
            switch (node.nodeType()) {
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

    //while( p_end > p  &&  isspace( (uchar)p_end[-1] ) )  p_end--;        // Alle Leerzeichen und Blanks etc. am Ende löschen

    text.erase( p_end - p0 );
    text.erase( 0, p - p0 );

    return text;
}

//--------------------------------------------------------------------------CharacterData_ptr::data

string CharacterData_ptr::data() const
{ 
    return _characterDataJ.getData();
}

//---------------------------------------------------------------------------Schema_ptr::Schema_ptr

Schema_ptr::Schema_ptr(const string& url)
:
    _validatorJ(CppXmlSchemaValidatorJ::new_instance(url))
{}

//-----------------------------------------------------------------------------Schema_ptr::validate

void Schema_ptr::validate( const Document_ptr& document )
{
    _validatorJ.validate(document.ref());
}

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer

#endif
