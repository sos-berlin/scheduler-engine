// $Id: xml_libxml2.h 14207 2011-03-31 11:26:18Z jz $

#ifndef __ZSCHIMMER_XML_LIBXML2_H
#define __ZSCHIMMER_XML_LIBXML2_H

#include "xml.h"
#include "../javaproxy/org__w3c__dom__Attr.h"
#include "../javaproxy/org__w3c__dom__CDATASection.h"
#include "../javaproxy/org__w3c__dom__CharacterData.h"
#include "../javaproxy/org__w3c__dom__Comment.h"
#include "../javaproxy/org__w3c__dom__Document.h"
#include "../javaproxy/org__w3c__dom__Element.h"
#include "../javaproxy/org__w3c__dom__Node.h"
#include "../javaproxy/org__w3c__dom__NodeList.h"
#include "../javaproxy/org__w3c__dom__Text.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__xml__CppXmlSchemaValidator.h"

typedef ::javaproxy::com::sos::scheduler::engine::kernel::xml::CppXmlSchemaValidator CppXmlSchemaValidatorJ;

//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace xml {

typedef ::javaproxy::org::w3c::dom::Attr AttrJ;
typedef ::javaproxy::org::w3c::dom::CharacterData CharacterDataJ;
typedef ::javaproxy::org::w3c::dom::CDATASection CDATASectionJ;
typedef ::javaproxy::org::w3c::dom::Comment CommentJ;
typedef ::javaproxy::org::w3c::dom::Document DocumentJ;
typedef ::javaproxy::org::w3c::dom::Element ElementJ;
typedef ::javaproxy::org::w3c::dom::Node NodeJ;
typedef ::javaproxy::org::w3c::dom::NodeList NodeListJ;
typedef ::javaproxy::org::w3c::dom::Text TextJ;

//-------------------------------------------------------------------------------------------------

const string                    default_character_encoding        = "ISO-8859-1";     // Eigentlich Windows-1252, aber das ist weniger bekannt und wir sollten die Zeichen 0xA0..0xBF nicht benutzen.

//-------------------------------------------------------------------------------------------------

struct Implementation_ptr;
struct ImplementationSource_ptr;
struct DocumentType_ptr;
struct Document_ptr;
struct DocumentFragment_ptr;
struct NodeList_ptr;
struct NamedNodeMap_ptr;        
struct UserDataHandler_ptr;
struct ErrorHandler_ptr;
struct Locator_ptr;
struct Element_ptr;
struct Text_ptr;
struct Comment_ptr;
struct CDATASection_ptr;
struct ProcessingInstruction_ptr;
struct EntityReference_ptr;
struct InputSource_ptr;
struct BuilderFilter_ptr;
struct WriterFilter_ptr;
struct XMLFormatTarget_ptr;
struct Node_list;

//-----------------------------------------------------------------------------------------NodeType

enum NodeType
{
                                            // Werte müssen mit libxml/tree.h übereinstimmen (das sind dieselben wie in http://www.w3.org/TR/REC-DOM-Level-1/)
    ELEMENT_NODE                    = 1,    //XML_ELEMENT_NODE,
    ATTRIBUTE_NODE                  = 2,    //XML_ATTRIBUTE_NODE,
    TEXT_NODE                       = 3,    //XML_TEXT_NODE,
    CDATA_SECTION_NODE              = 4,    //XML_CDATA_SECTION_NODE,
    ENTITY_REFERENCE_NODE           = 5,    //XML_ENTITY_REF_NODE,
    ENTITY_NODE                     = 6,    //XML_ENTITY_NODE,
    PROCESSING_INSTRUCTION_NODE     = 7,    //XML_PI_NODE,
    COMMENT_NODE                    = 8,    //XML_COMMENT_NODE,
    DOCUMENT_NODE                   = 9,    //XML_DOCUMENT_NODE,
    DOCUMENT_TYPE_NODE              = 10,   //XML_DOCUMENT_TYPE_NODE,
    DOCUMENT_FRAGMENT_NODE          = 11,   //XML_DOCUMENT_FRAG_NODE,
    NOTATION_NODE                   = 12,   //XML_NOTATION_NODE
};

//-------------------------------------------------------------------------------------------------

string                          name_of_node_type           ( const NodeType& );

//----------------------------------------------------------------------------------Simple_node_ptr

struct Simple_node_ptr
{
                                Simple_node_ptr             ()                                       {}
                                Simple_node_ptr             (const NodeJ& o)                        : _nodeJ(o) {}
                                Simple_node_ptr             (const Simple_node_ptr& o)              : _nodeJ(o._nodeJ) {}

    virtual                    ~Simple_node_ptr             ()                                      {}              // Für gcc 3.2


  //void                        free                        ();
    virtual void                assign                      (const NodeJ& o)                        { _nodeJ = o; }
    virtual void                assign                      (const NodeJ&, NodeType );

    const NodeJ&                ref                         () const                                { return _nodeJ; }

                                //operator const NodeJ&     () const                                { return node(); }
                                operator bool               () const                                { return !!_nodeJ; }

    virtual bool                is_type                     ( NodeType )                            { return false; }
    void                        assert_type                 ( NodeType );
    void                        throw_node_type             ( NodeType ) const;

    string                      nodeName                    () const;

    bool                        nodeName_is                 ( const char* name ) const;
    bool                        nodeName_is                 ( const string& name ) const            { return nodeName_is( name.c_str() ); }

    void                    set_nodeName                    ( const string& ) const;

    bool                        contains_node               (const Simple_node_ptr& node) const;
    
    string                      nodeValue                   () const;
    NodeType                    nodeType                    () const;
    Simple_node_ptr             parentNode                  () const;
    Simple_node_ptr             firstChild                  () const;
    Simple_node_ptr             lastChild                   () const;
    Simple_node_ptr             nextSibling                 () const;
    Document_ptr                ownerDocument               () const;
    Simple_node_ptr             cloneNode                   ( bool deep ) const;
    Simple_node_ptr             insertBefore                ( const Simple_node_ptr& newChild, const Simple_node_ptr& refChild ) const;
    Simple_node_ptr             replaceChild                ( const Simple_node_ptr& newChild, const Simple_node_ptr& oldChild ) const;
    Simple_node_ptr             replace_with                ( const Simple_node_ptr& );
    void                        removeChild                 ( const Simple_node_ptr& child ) const;
    Simple_node_ptr             appendForeignChild          (const Simple_node_ptr&) const;
    Simple_node_ptr             adoptAndAppendChild         (const Simple_node_ptr&) const;    
    Simple_node_ptr             appendChild                 ( const Simple_node_ptr& newChild ) const;
    Simple_node_ptr             appendChild_if              ( const Simple_node_ptr& newChild ) const             { return newChild? appendChild( newChild ) : Simple_node_ptr(NULL); }
    bool                        hasChildNodes               () const;
    string                      getTextContent              () const;
    void                        setTexContent              ( const string& textContent ) const;
    string                      xml_without_prolog          ( const string& encoding, bool indented = false ) const;
    string                      xml                         ( const string& encoding, bool indented = false ) const;
    string                      xml                         () const                                { return xml( default_character_encoding, false ); }

    int                         line_number                 () const;
    Node_list                   select_nodes                ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node                 ( const string& xpath_expression ) const;
    bool                        has_node                    ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node_strict          ( const string& xpath_expression ) const;
    Element_ptr                 select_element_strict       ( const string& xpath_expression ) const;

private:
    DocumentJ                   thisOrOwnerDocumentJ        () const;

    NodeJ                      _nodeJ;
};

//-------------------------------------------------------------------------------------Document_ptr

struct Document_ptr : Simple_node_ptr
{
                                Document_ptr                ( const DocumentJ& doc = NULL )         { assign( doc ); }
                                Document_ptr                ( const Document_ptr& doc )             { assign( doc.ref() ); }
                                Document_ptr                ( const string& xml, const string& encoding = "" ) { load_xml( xml, encoding ); }
                                Document_ptr                ( const BSTR xml )                      { load_xml( xml ); }

    Document_ptr&               operator =                  ( const Document_ptr& doc )             { assign( doc.ref() );  return *this; }
    Document_ptr&               operator =                  (const DocumentJ& doc )                 { assign( doc       );  return *this; }

    void                        assign                      (const DocumentJ& doc );

    const DocumentJ&            ref                         () const                                { return _documentJ; }

    //const DocumentJ&            detach                      ()                                      { const DocumentJ& result = ref(); _nodeJ = NULL; return result; }

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == DOCUMENT_NODE || Simple_node_ptr::is_type( type ); }

    Document_ptr&               create                      ();
    bool                        try_load_xml                ( const string& text, const string& encoding = "" );
    bool                        try_load_xml                ( const BSTR text );
    void                        load_xml                    ( const string& text, const string& encoding = "" );
    void                        load_xml                    ( const BSTR text );
    string                      xml                         ( const string& encoding, const string& indent_string = "" ) const;
    string                      xml                         () const                                { return xml( default_character_encoding ); }

    Element_ptr                 createElement               ( const string& tagName ) const;
    Text_ptr                    createTextNode              ( const string& data ) const;
    Comment_ptr                 createComment               ( const string& data ) const;
    CDATASection_ptr            createCDATASection          ( const string& data ) const;
    ProcessingInstruction_ptr   createProcessingInstruction ( const string& target, const string& data ) const;

    Simple_node_ptr             importNode                  (const Simple_node_ptr&) const;
    
    Element_ptr                 documentElement             () const;

    Element_ptr                 create_root_element         ( const string& name );

private:
    DocumentJ                  _documentJ;
};                                                          

//-----------------------------------------------------------------------------------------Node_ptr
// _document hält den Speicher des Node_ptr, mit meiner Änderung des libxml2-Codes 

struct Node_ptr : Simple_node_ptr
{
                                Node_ptr                    ()                                      {}
                                Node_ptr                    ( const NodeJ& o )                      : Simple_node_ptr(o)   {}
                                Node_ptr                    ( const Simple_node_ptr& o )            : Simple_node_ptr(o.ref())  {}

                                operator const NodeJ&       () const                                { return ref(); }

  //void                        free                        ()                                      { Simple_node_ptr::free(); }
    void                        assign                      (const NodeJ& o)                        { Simple_node_ptr::assign(o); }
    void                        assign                      (const NodeJ& o, NodeType t )           { Simple_node_ptr::assign(o, t); }

    Document_ptr               _document;
};

//-------------------------------------------------------------------------------------NodeList_ptr
/*
struct NodeList_ptr : Node_ptr
{
                                NodeList_ptr                ( _xmlNode* p )                         :  Node_ptr( p ) {}

    _xmlNode*                   ptr                         ()                                      { return (_xmlNode*)_ptr; }
};
*/
//--------------------------------------------------------------------------------CharacterData_ptr

struct CharacterData_ptr : Node_ptr 
{
                                CharacterData_ptr           (const CharacterDataJ& o)               : Node_ptr(NodeJ(o)), _characterDataJ(o) {}
                                CharacterData_ptr           (const Simple_node_ptr& o)              : Node_ptr(o), _characterDataJ(o.ref()) {}
                                CharacterData_ptr           (const CharacterData_ptr& o )           : Node_ptr(NodeJ(o.ref())), _characterDataJ(o.ref()) {}

    string                      data                        () const;

private:
    const CharacterDataJ       _characterDataJ;
};

//-----------------------------------------------------------------------------------------Text_ptr

struct Text_ptr : CharacterData_ptr
{
                                Text_ptr                    (const TextJ& o = NULL )                : CharacterData_ptr(CharacterDataJ(o)) {}
                              //Text_ptr                    (const NodeJ& o = NULL )                : Node_ptr(o) {}
                                Text_ptr                    ( const Simple_node_ptr& p )            : CharacterData_ptr(p) {}

                              //Text_ptr                    ( const Text_ptr& p )                   : Node_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == TEXT_NODE || Node_ptr::is_type( type ); }
};

//--------------------------------------------------------------------------------------Element_ptr

struct Element_ptr : Node_ptr
{
    enum No_xc { no_xc };

                                Element_ptr                 ( const ElementJ& o = NULL )            : Node_ptr(NodeJ(o)), _elementJ(o) {}
                                Element_ptr                 ( const Simple_node_ptr& p )            : Node_ptr(p.ref()), _elementJ(p.ref()) {}
    explicit                    Element_ptr                 ( const Simple_node_ptr& p, No_xc )     : Node_ptr(p.ref()), _elementJ(NULL) { if (is_type( ELEMENT_NODE)) _elementJ = p.ref(); else assign(NULL); }

    void                        assign                      ( const ElementJ& o)                    { _elementJ = o; Node_ptr::assign(NodeJ(o)); }

  //_xmlElement*                ptr                         () const                                { return (_xmlElement*)_ptr; }

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == ELEMENT_NODE || Node_ptr::is_type( type ); }

    string                      getAttribute                ( const char* name, const char* deflt = "" ) const;
    string                      getAttribute_mandatory      ( const string& name ) const;
    string                      getAttribute                ( const string& name, const string& deflt = "" ) const;
    bool                   bool_getAttribute                ( const string& name, bool deflt = false ) const;
    int                     int_getAttribute                ( const string& name ) const;
    int                     int_getAttribute                ( const string& name, int deflt ) const;
    int                    uint_getAttribute                ( const string& name, int deflt ) const;
    void                        setAttribute_optional       ( const string& name, const string& value ) const { if( value != "" )  setAttribute( name, value ); }
    void                        setAttribute                ( const string& name, const string& value ) const { setAttribute( name, value.c_str() ); }
    void                        setAttribute_optional       ( const string& name, const BSTR    value ) const { if( SysStringLen(value) > 0 )  setAttribute( name, value ); }
    void                        setAttribute                ( const string& name, const BSTR    value ) const { setAttribute( name, com::string_from_bstr( value ) ); }
    void                        setAttribute                ( const string& name, const char*   value ) const;
    void                        setAttribute                ( const string& name, bool value ) const;
    void                        setAttribute                ( const string& name, int64 value ) const;
    void                        setAttribute                ( const string& name, int value ) const           { setAttribute( name, (int64)value ); }  // Für gcc 3.3
    void                        setAttribute                ( const string& name, long value ) const          { setAttribute( name, (int64)value ); }  // Für gcc 3.3
    bool                        hasAttributes               () const;
    void                        removeAttribute             ( const string& name ) const;
    bool                        hasAttribute                ( const string& name ) const;
    Element_ptr                 first_child_element         () const;
    Element_ptr                 append_new_element          ( const string& element_name ) const;
    Element_ptr                 append_new_text_element     ( const string& element_name, const string& text ) const;             
    Element_ptr                 append_new_text_element     ( const string& element_name, const BSTR text ) const { return append_new_text_element( element_name, com::string_from_bstr( text ) ); }
    Element_ptr                 append_new_cdata_element    ( const string& element_name, const string& text ) const;
    Element_ptr                 append_new_cdata_or_text_element( const string& element_name, const string& text ) const;
    Element_ptr                 append_new_name_value_element_optional( const string& element_name, 
                                                              const string& name_attribute_name, const string& name_value, 
                                                              const string& value_attribute_name, const string& value_value );
    Element_ptr                 append_new_name_value_element_optional( const char* element_name, 
                                                              const char* name_attribute_name, const char* name_value, 
                                                              const char* value_attribute_name, const string& value_value );
    Comment_ptr                 append_new_comment          ( const string& );

    string                      text                        () const;
    string                      trimmed_text                () const;                               // Erste und letzte Leerzeile werden abgeschnitten (nach <tag> und vor </tag>)

private:
    ElementJ                   _elementJ;
};

//--------------------------------------------------------------------------------------Comment_ptr

struct Comment_ptr : CharacterData_ptr
{
                                Comment_ptr                 (const CommentJ& o)                     : CharacterData_ptr(CharacterDataJ(o)) {}
                                Comment_ptr                 (const Simple_node_ptr& p)              : CharacterData_ptr(p) {}
                                Comment_ptr                 (const Comment_ptr& o)                  : CharacterData_ptr(CharacterDataJ(o.ref())) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == COMMENT_NODE || CharacterData_ptr::is_type( type ); }
};

//---------------------------------------------------------------------------------CDATASection_ptr

struct CDATASection_ptr : Text_ptr
{
                                CDATASection_ptr            (const CDATASectionJ& o)                : Text_ptr(TextJ(o)) {}
                                CDATASection_ptr            ( const Simple_node_ptr& p )            : Text_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == CDATA_SECTION_NODE || Text_ptr::is_type( type ); }
};

//----------------------------------------------------------------------------------------Node_list

struct Node_list {
                                Node_list                   ()                                      {}
                                Node_list                   (const NodeListJ& o)                    : _nodeListJ(o) {}

    int                         count                       () const;
    Simple_node_ptr             operator[]                  (int i) const                           { return get(i); }
    Simple_node_ptr             get                         (int i) const;
                                                                                                                                 
private:
    const NodeListJ            _nodeListJ;
};

//------------------------------------------------------------------------ProcessingInstruction_ptr
/*
struct ProcessingInstruction_ptr : Node_ptr 
{
                                ProcessingInstruction_ptr   (const NodeJ& p = NULL )                  : Node_ptr(p) {}
                                ProcessingInstruction_ptr   ( const Simple_node_ptr& p )            : Node_ptr(p) {}
                                ProcessingInstruction_ptr   ( const ProcessingInstruction_ptr& p )  : Node_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == PROCESSING_INSTRUCTION_NODE || Node_ptr::is_type( type ); }

    string                      getTarget                   () const;
    string                      getData                     () const;
    void*                       setData                     ( const string& data );
};
*/
//---------------------------------------------------------------------------------------Schema_ptr

struct Schema_ptr
{
                                Schema_ptr                  ()                                      {}
                                Schema_ptr                  (const string& url);
    void                        validate                    ( const Document_ptr& );

    CppXmlSchemaValidatorJ     _validatorJ;
};

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer

#endif
