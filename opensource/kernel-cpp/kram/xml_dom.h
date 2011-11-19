// $Id: xml_dom.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __SOS_XML_DOM_H
#define __SOS_XML_DOM_H

//----------------------------------------------------------------------------------------DECL_TYPE

#define DECL_TYPE( TYPE )  namespace MSXML2 { struct IXMLDOM##TYPE; }  typedef MSXML2::IXMLDOM##TYPE Xml_dom_##TYPE;

//----------------------------------------------------------------------------------------DECL_TYPE

//#define DELEGATOR_STD_DECL( TYPE )  TYPE() : _delegator

//------------------------------------------------------------------------------DELEGATED_READ_PROP
/*
#define DELEGATED_READ_PROP( RESULT_TYPE, TYPE, NAME )                          \
    RESULT_TYPE TYPE::NAME()                                                    \
    {                                                                           \
        RESULT_TYPE result;                                                     \
                                                                                \
        result._delegated = delegated()->Get##NAME();                           \
                                                                                \
        return result;                                                          \
    }
*/
//------------------------------------------------------------------------------DELEGATED_READ_PROP

#define DELEGATED_READ_PROP( RESULT_TYPE, TYPE, NAME )                          \
    RESULT_TYPE TYPE::NAME()                                                    \
    {                                                                           \
        RESULT_TYPE result;                                                     \
                                                                                \
        HRESULT hr = delegated()->get_##NAME( (MSXML2::IXMLDOM##RESULT_TYPE**)&result._delegated );  \
        if( FAILED(hr) )  throw_ole( hr, "DOM" #TYPE "::" #NAME );              \
                                                                                \
        return result;                                                          \
    }

//-----------------------------------------------------------------------DELEGATED_READ_PROP_SIMPLE

#define DELEGATED_READ_PROP_SIMPLE( RESULT_TYPE, TYPE, NAME )                   \
    RESULT_TYPE TYPE::NAME()                                                    \
    {                                                                           \
        return delegated()->NAME;                                    \
    }

//-----------------------------------------------------------------------DELEGATED_READ_PROP_STRING

#define DELEGATED_READ_PROP_STRING( TYPE, NAME )                                \
    string TYPE::NAME()                                                         \
    {                                                                           \
        return w_as_string( delegated()->Get##NAME() );                         \
    }

/*
#ifdef SYSTEM_WIN
    namespace MSXML2 
    {
        struct IXMLDOMDocument2;
        struct IXMLDOMElement;
        struct IXMLDOMNodeList;
        struct IXMLDOMNamedNodeMap;
    }

    typedef MSXML2::IXMLDOMDocument2        Xml_dom_document;
    typedef MSXML2::IXMLDOMElement          Xml_dom_element;
    typedef MSXML2::IXMLDOMNodeList         Xml_dom_node_list;
    typedef MSXML2::IXMLDOMNamedNodeMap     Xml_dom_named_node_map;
#endif
*/

struct IUnknown;

DECL_TYPE( Document )
DECL_TYPE( Node )
DECL_TYPE( Element )
DECL_TYPE( NodeList )
DECL_TYPE( NamedNodeMap )

namespace sos {
namespace xml {
namespace dom {


struct Node;

//----------------------------------------------------------------------------------------Delegator

struct Delegator
{
                                Delegator               ( IUnknown* p = NULL )      : _delegated(p) {}
                                Delegator               ( const Delegator& );
    virtual                    ~Delegator               ();

                                operator void*          ()                          { return _delegated; }
    bool                        operator !              ()                          { return _delegated == NULL; }

    IUnknown*                   get_delegated           ( const void* iid );
    IUnknown*                  _delegated;
};

//---------------------------------------------------------------------------------------ParseError

struct ParseError
{
};

//-----------------------------------------------------------------------------------------NodeType

enum NodeType
{
    NODE_ELEMENT                = 1,
    NODE_ATTRIBUTE              = 2,
    NODE_TEXT                   = 3,
    NODE_CDATA_SECTION          = 4,
    NODE_ENTITY_REFERENCE       = 5,
    NODE_ENTITY                 = 6,
    NODE_PROCESSING_INSTRUCTION = 7,
    NODE_COMMENT                = 8,
    NODE_DOCUMENT               = 9,
    NODE_DOCUMENT_TYPE          = 10,
    NODE_DOCUMENT_FRAGMENT      = 11,
    NODE_NOTATION               = 12
};

//-------------------------------------------------------------------------------------NamedNodeMap 

struct NamedNodeMap : Delegator
{
    Xml_dom_NamedNodeMap*       delegated               (); //                          { return (Xml_dom_NamedNodeMap*&)_delegated; }
};

//-----------------------------------------------------------------------------------------NodeList

struct NodeList : Delegator
{
    long                        length                  ();
    Node                        item                    ( int index );

    Xml_dom_NodeList*           delegated               (); //                          { return (Xml_dom_NodeList*&)_delegated; }
    Xml_dom_NodeList**          delegated_ptr           ()                          { return &(Xml_dom_NodeList*&)_delegated; }
};

//---------------------------------------------------------------------------------------------Node

struct Node : Delegator
{
    NodeType                    nodeType                ();
    NodeList                    childNodes              ();
  //Node                        selectSingleNode        ( const string& query );
    string                      nodeName                ();
    string                      text                    ();

    Xml_dom_Node*               delegated               (); //                          { return (Xml_dom_Node*&)_delegated; }
    Xml_dom_Node**              delegated_ptr           ()                          { return &(Xml_dom_Node*&)_delegated; }
};

//------------------------------------------------------------------------------------------Element

struct Element : Node
{
                                Element                 ()                          {}
                                Element                 ( Node& );

    Xml_dom_Element*            delegated               ();//                          { return (Xml_dom_Element*&)_delegated; }
    Xml_dom_Element**           delegated_ptr           ()                          { return &(Xml_dom_Element*&)_delegated; }

  //NodeList                    childNodes              ();
    string                      getAttribute            ( const string& name );
    NamedNodeMap                attributes              ();
    NodeList                    getElementsByTagName    ( const string& name );
    Element                     single_element          ( const string& name );
    string                      element_text            ( const string& name );
};

//-----------------------------------------------------------------------------------------Document

struct Document : Element
{
                                Document                ();
                               ~Document                ();

    void                        loadXML                 ( const string& );
    Element                     documentElement         ();
  //NodeList                    childNodes              ();

    Xml_dom_Document*           delegated               (); //                          { return (Xml_dom_Document*)_delegated; }
};

//-------------------------------------------------------------------------------------------------

} //namespace dom
} //namespace xml
} //namespace sos

#endif
