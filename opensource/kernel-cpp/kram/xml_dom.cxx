// $Id: xml_dom.cxx 11394 2005-04-03 08:30:29Z jz $       

#include "precomp.h"
#include "sos.h"
#include "xml_dom.h"

#ifdef SYSTEM_WIN

#include "olestd.h"
#include <windows.h>
#include <ole2.h>

//#   include "../misc/microsoft/msxml/msxml2.h"
#import <msxml3.dll> //raw_interfaces_only 
using namespace MSXML2;

namespace sos {
namespace xml {
namespace dom {

//Ole_class_descr* sax_content_handler_for_com_class_ptr;

//-------------------------------------------------------------------------------Attributes::length
/*
int Attributes::length()
{
    int len;

    HRESULT hr = _delegated->getLength( &len );
    
    if( FAILED(hr) )  throw_ole( hr, "ISAXAttributes::getLength" );

    return len;
}

//---------------------------------------------------------------------------Attributes::local_name

string Attributes::local_name( int index )
{
    wchar_t* name;
    int      len;

    HRESULT hr = _delegated->getLocalName( index, &name, &len );

    if( FAILED(hr) )  throw_ole( hr, "ISAXAttributes::getLocalName" );

    return w_as_string( name, len );
}

//--------------------------------------------------------------------------------Attributes::value

string Attributes::value( int index )
{
    wchar_t* name;
    int      len;

    HRESULT hr = _delegated->getValue( index, &name, &len );

    if( FAILED(hr) )  throw_ole( hr, "ISAXAttributes::getValue" );

    return w_as_string( name, len );
}
*/
//-----------------------------------------------------------------------------Delegator::Delegator

Delegator::Delegator( const Delegator& d )
{
    _delegated = d._delegated;

    if( _delegated )  ((IUnknown*)_delegated)->AddRef();
}

//----------------------------------------------------------------------------Delegator::~Delegator

Delegator::~Delegator()
{
    if( _delegated )  ((IUnknown*)_delegated)->Release();
}

//-------------------------------------------------------------------------Delegator::get_delegated

IUnknown* Delegator::get_delegated( const void* iid )
{
/*
    IUnknown* o;

    HRESULT hr = _delegated->QueryInterface( *(IID*)iid, (void**)&o );
    if( FAILED(hr) )  throw_ole( hr, "QueryInterface" );

    return o;
*/
    return _delegated;
}

//---------------------------------------------------------------------------------------------Node

DELEGATED_READ_PROP         ( NodeList    , Node,       childNodes )
DELEGATED_READ_PROP         ( NamedNodeMap, Element,    attributes )
DELEGATED_READ_PROP_SIMPLE  ( long        , NodeList,   length )
DELEGATED_READ_PROP_STRING  (               Node,       nodeName )
DELEGATED_READ_PROP_STRING  (               Node,       text )
//DELEGATED_READ_PROP         ( NodeList    , Element,    childNodes )
//DELEGATED_READ_PROP         ( NodeList    , Document,   childNodes )

//----------------------------------------------------------------------------Node::selectSingleNode
/* Nur Microsoft:
Node Node::selectSingleNode( const string& query )
{
    Bstr query_bstr;
    Node result;

    query_bstr.Attach( SysAllocString_string( query ) );

    HRESULT hr = delegated()->selectSingleNode( query_bstr, &result.delegated() );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::get_baseName" );

    return result;
}
*/
//--------------------------------------------------------------------------------Element::baseName
/*
string Element::baseName()
{
    Bstr result_bstr;

    HRESULT hr = delegated()->get_baseName( &result_bstr );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::get_baseName" );

    return as_string( result_bstr );
}
*/
//-----------------------------------------------------------------------------------NodeList::item

Node NodeList::item( int i )
{
    Node node;

    HRESULT hr = delegated()->get_item( i, node.delegated_ptr() );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::item" );

    return node;
}

//-----------------------------------------------------------------------------------Node::delegated

IXMLDOMNode*        Node::delegated()       { return (IXMLDOMNode*)     get_delegated(&__uuidof(IXMLDOMNode)); }
IXMLDOMNodeList*    NodeList::delegated()   { return (IXMLDOMNodeList*) get_delegated(&__uuidof(IXMLDOMNodeList)); }
IXMLDOMElement*     Element::delegated()    { return (IXMLDOMElement*)  get_delegated(&__uuidof(IXMLDOMElement)); }
IXMLDOMDocument*    Document::delegated()   { return (IXMLDOMDocument*) get_delegated(&__uuidof(IXMLDOMDocument)); }

//------------------------------------------------------------------------------------Node::nodeType

NodeType Node::nodeType()
{
    DOMNodeType result;

    HRESULT hr = delegated()->get_nodeType( &result );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::nodeType" );

    return (NodeType)result;
}

//----------------------------------------------------------------------------------Element::Element

Element::Element( Node& node )
:
    Node( node )
{
    if( node.nodeType() != NODE_ELEMENT )  throw_xc( "XML-NO-NODE_ELEMENT" );
}

//-----------------------------------------------------------------------------Element::getAttribute

string Element::getAttribute( const string& name )
{
    Bstr    name_bstr;
    Variant result_vt;

    name_bstr.Attach( SysAllocString_string( name ) );

    result_vt = delegated()->getAttribute( name.c_str() );
    //HRESULT hr = delegated()->getAttribute( name_bstr, &result_vt );
    //if( FAILED(hr) )  throw_ole( hr, "DOMDocument::getAttribute" );

    return as_string( result_vt );
}

//-----------------------------------------------------------------------------Element::element_text

string Element::element_text( const string& name )
{
    NodeList node_list = getElementsByTagName( name );

    if( node_list.length() == 0 )  return "";

    return node_list.item(0).text();
}

//---------------------------------------------------------------------Element::getElementsByTagName

NodeList Element::getElementsByTagName( const string& name )
{
    Bstr     name_bstr;
    NodeList result;

    name_bstr.Attach( SysAllocString_string( name ) );

    //*result.delegated_ptr() = delegated()->getElementsByTagName( name.c_str() );
    HRESULT hr = delegated()->raw_getElementsByTagName( name_bstr, result.delegated_ptr() );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::getElementsByTagName" );

    return result;
}

//--------------------------------------------------------------------------Element::single_element

Element Element::single_element( const string& name )
{
    NodeList list = getElementsByTagName( name );

    int len = list.length();
    if( len == 0 )  throw_xc( "SOS-1422", name );
    if( len > 1 )  throw_xc( "SOS-1423", name );

    return list.item(0);
}

//------------------------------------------------------------------------------Element::childNodes
/*
Node_list Element::childNodes()
{
    Node_list result;

    HRESULT hr = _delegated->get_childNodes( &result->_delegated );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::get_nodeList" );

    return result;
}
*/
//-------------------------------------------------------------------------------Document::Document

Document::Document()
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    hr = CoCreateInstance( __uuidof(DOMDocument30), NULL, CLSCTX_ALL, __uuidof(IXMLDOMDocument2), (void**)&_delegated );
    if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance(SAXXMLReader)" );
}

//------------------------------------------------------------------------------Document::~Document

Document::~Document()
{
    //CoUninitialize();
}

//------------------------------------------------------------------------Document::documentElement

DELEGATED_READ_PROP( Element, Document, documentElement )
/*
Element Document::documentElement()
{
    Element element;

    HRESULT hr = _delegated->get_documentElement( &element._delegated );
    if( FAILED(hr) )  throw_ole( hr, "DOMDocument::get_documentElement" );

    return element;
}
*/
//--------------------------------------------------------------------------------Document::loadXML

void Document::loadXML( const string& text )
{
    VARIANT_BOOL  ok;

    ok = delegated()->loadXML( text.c_str() );

    if( !ok ) 
    { 
        IXMLDOMParseErrorPtr error = delegated()->GetparseError();

        string text = w_as_string( error->reason );
        text += ", code="   + as_string( error->errorCode );
        text += ", line="   + as_string( error->line );
        text += ", column=" + as_string( error->linepos );

        throw_xc( "XML-ERROR", text );
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace dom
} //namespace xml
} //namespace sos

#else
    // Unix libxml
#endif