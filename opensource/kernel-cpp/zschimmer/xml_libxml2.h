// $Id: xml_libxml2.h 14207 2011-03-31 11:26:18Z jz $

#ifndef __ZSCHIMMER_XML_LIBXML2_H
#define __ZSCHIMMER_XML_LIBXML2_H

/*
    Implementierung ähnlich http://www.w3.org/TR/2002/WD-DOM-Level-3-Core-20021022/core.html
*/

#include "xml.h"
#include "z_com.h"

//--------------------------------------------------------------------------------Typen von libxml2

struct _xsltStylesheet;
struct _xmlDoc;
struct _xmlDtd;
struct _xmlSchema;
struct _xmlElement;
struct _xmlNode;
struct _xmlAttr;
struct _xmlXPathContext;
struct _xmlXPathObject;
typedef unsigned char    xmlChar;       // xmlChar* ist ein UTF-8-String

//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace xml {
namespace libxml2 {

//-------------------------------------------------------------------------------------------------

const string                    default_character_encoding        = "ISO-8859-1";     // Eigentlich Windows-1252, aber das ist weniger bekannt und wir sollten die Zeichen 0xA0..0xBF nicht benutzen.

//-------------------------------------------------------------------------------------------------


struct Attr_ptr;
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
struct Xpath_nodes;

//-------------------------------------------------------------------------------Libxml2_error_text

struct Libxml2_error_text
{
    const string&               error_text                  () const                                { return _error_text; }

    string                     _error_code;
    string                     _error_text;
};

//------------------------------------------------------------------------------Activate_error_text

struct Activate_error_text
{
                                Activate_error_text         ( Libxml2_error_text* );
                               ~Activate_error_text         ();
};

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

//--------------------------------------------------------------------------------------Utf8_string

struct Utf8_string
{
                                Utf8_string                 ( const char* s   )                     { set_latin1( s, strlen( s )        ); }
                                Utf8_string                 ( const string& s )                     { set_latin1( s.c_str(), s.length() ); }

    void                        set_latin1                  ( const char*, size_t length );
    xmlChar*                    utf8                        () const                                { return (xmlChar*)_utf8.c_str(); }
    size_t                      byte_count                  () const                                { return _utf8.length(); }

    string                     _utf8;
};


typedef xmlChar* DOMString;

//-------------------------------------------------------------------------------------------------

string                          name_of_node_type           ( const NodeType& );
string                          sd                          ( const xmlChar* );
string                          sd_free                     ( xmlChar* );

//----------------------------------------------------------------------------------Simple_node_ptr

struct Simple_node_ptr
{
                                Simple_node_ptr             ()                                      : _ptr(NULL) {}
                                Simple_node_ptr             ( _xmlNode* ptr )                       : _ptr(ptr) {}
                                Simple_node_ptr             ( const Simple_node_ptr& p )            : _ptr(p._ptr) {}

    virtual                    ~Simple_node_ptr             ()                                      {}              // Für gcc 3.2


    virtual void                free                        ();
    virtual void                assign                      ( _xmlNode* ptr )                       { _ptr = ptr; }
    virtual void                assign                      ( _xmlNode*, NodeType );

    _xmlNode*                   ptr                         () const                                { return _ptr; }

                                operator _xmlNode*          () const                                { return ptr(); }

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
    Simple_node_ptr             appendChild                 ( const Simple_node_ptr& newChild ) const;
    Simple_node_ptr             appendChild_if              ( const Simple_node_ptr& newChild ) const             { return newChild? appendChild( newChild ) : NULL; }
    bool                        hasChildNodes               () const;
    string                      getTextContent              () const;
    void                        setTexContent              ( const string& textContent ) const;
    string                      xml_without_prolog          ( const string& encoding, bool indented = false ) const;
    string                      xml                         ( const string& encoding, bool indented = false ) const;
    string                      xml                         () const                                { return xml( default_character_encoding, false ); }

    int                         line_number                 () const;
    Xpath_nodes                 select_nodes                ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node                 ( const string& xpath_expression ) const;
    bool                        has_node                    ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node_strict          ( const string& xpath_expression ) const;
    Element_ptr                 select_element_strict       ( const string& xpath_expression ) const;


    _xmlNode*                   _ptr;
};

//-------------------------------------------------------------------------------------Document_ptr

struct Document_ptr : Simple_node_ptr, 
                      Libxml2_error_text
{
                                Document_ptr                ( _xmlDoc* doc = NULL )                 { assign( doc ); }
                                Document_ptr                ( const Document_ptr& doc )             { assign( doc.ptr() ); }
                                Document_ptr                ( const string& xml, const string& encoding = "" ) { load_xml( xml, encoding ); }
                                Document_ptr                ( const BSTR xml )                      { load_xml( xml ); }
                               ~Document_ptr                ()                                      { release(); }

    Document_ptr&               operator =                  ( const Document_ptr& doc )             { assign( doc.ptr() );  return *this; }
    Document_ptr&               operator =                  ( _xmlDoc* doc )                        { assign( doc       );  return *this; }

    void                        assign                      ( _xmlDoc* doc );

    void                        release                     ();

    _xmlDoc*                    ptr                         () const                                { return (_xmlDoc*)_ptr; }

    _xmlDoc*                    detach                      ()                                      { _xmlDoc* result = ptr(); _ptr = NULL; return result; }

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == DOCUMENT_NODE || Simple_node_ptr::is_type( type ); }

    Document_ptr&               create                      ();
    bool                        try_load_xml                ( const string& text, const string& encoding = "" );
    bool                        try_load_xml                ( const BSTR text );
    void                        load_xml                    ( const string& text, const string& encoding = "" );
    void                        load_xml                    ( const BSTR text );
    string                      xml                         ( const string& encoding, const string& indent_string = "" ) const;
    string                      xml                         () const                                { return xml( default_character_encoding ); }

    Element_ptr                 createElement               ( const string& tagName ) const;
    Element_ptr                 createElement               ( const char* tagName ) const;
    Text_ptr                    createTextNode              ( const string& data ) const;
    Comment_ptr                 createComment               ( const string& data ) const;
    Comment_ptr                 createComment               ( const char* data ) const;
    CDATASection_ptr            createCDATASection          ( const string& data ) const;
    ProcessingInstruction_ptr   createProcessingInstruction ( const string& target, const string& data ) const;

    Simple_node_ptr             clone                       ( const Simple_node_ptr& node, int extended = 1 ) const;
    
    Element_ptr                 documentElement             () const;

    Element_ptr                 create_root_element         ( const string& name );
};                                                          

//-----------------------------------------------------------------------------------------Node_ptr
// _document hält den Speicher des Node_ptr, mit meiner Änderung des libxml2-Codes 

struct Node_ptr : Simple_node_ptr
{
                                Node_ptr                    ()                                      {}
                                Node_ptr                    ( _xmlNode* ptr )                       : Simple_node_ptr( ptr )   { set_document(); }
                                Node_ptr                    ( const Simple_node_ptr& p )            : Simple_node_ptr( p._ptr) { set_document(); }

                                operator _xmlNode*          () const                                { return ptr(); }

    void                        free                        ()                                      { Simple_node_ptr::free();  set_document(); }
    void                        assign                      ( _xmlNode* ptr )                       { Simple_node_ptr::assign( ptr ); set_document(); }
    void                        assign                      ( _xmlNode* ptr, NodeType nt )          { Simple_node_ptr::assign( ptr, nt ); set_document(); }
    bool                        is_orphan                   () const;

    void                    set_document                    ();

    Document_ptr               _document;
};

//-------------------------------------------------------------------------------------NodeList_ptr

struct NodeList_ptr : Node_ptr
{
                                NodeList_ptr                ( _xmlNode* p )                         :  Node_ptr( p ) {}

    _xmlNode*                   ptr                         ()                                      { return (_xmlNode*)_ptr; }
};

//--------------------------------------------------------------------------------CharacterData_ptr

struct CharacterData_ptr : Node_ptr 
{
                                CharacterData_ptr           ( _xmlNode* p = NULL )                  : Node_ptr(p) {}
                                CharacterData_ptr           ( const Simple_node_ptr& p )            : Node_ptr(p) {}
                                CharacterData_ptr           ( const CharacterData_ptr& p )          : Node_ptr(p) {}
};

//-----------------------------------------------------------------------------------------Text_ptr

struct Text_ptr : CharacterData_ptr
{
                                Text_ptr                    ( _xmlNode* p = NULL )                  : CharacterData_ptr(p) {}
                                Text_ptr                    ( const Simple_node_ptr& p )            : CharacterData_ptr(p) {}

                                Text_ptr                    ( const Text_ptr& p )                   : CharacterData_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == TEXT_NODE || CharacterData_ptr::is_type( type ); }

    string                      data                        () const;
};

//-----------------------------------------------------------------------------------------Attr_ptr

struct Attr_ptr : Node_ptr
{
                                Attr_ptr                    ( _xmlAttr* ptr )                       : Node_ptr( (_xmlNode*)ptr ) {} 

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == ATTRIBUTE_NODE || Attr_ptr::is_type( type ); }

    string                      name                        ();
    bool                        specified                   ();
    string                      value                       ();
    void                    set_value                       ( const string& value );
    Element_ptr                 ownerElement                ();
};

//--------------------------------------------------------------------------------------Element_ptr

struct Element_ptr : Node_ptr
{
    enum No_xc { no_xc };

                                Element_ptr                 ( _xmlElement* ptr = NULL )             : Node_ptr( (_xmlNode*)ptr ) {}
    explicit                    Element_ptr                 ( _xmlNode* p )                         { assign( p ); }
    explicit                    Element_ptr                 ( _xmlNode* p, No_xc )                  : Node_ptr( p ) { if( !is_type( ELEMENT_NODE ) )  _ptr = NULL; }
                                Element_ptr                 ( const Simple_node_ptr& p )            { assign( p ); }

    void                        assign                      ( _xmlElement* ptr )                    { Node_ptr::assign( (_xmlNode*)ptr ); }
    void                        assign                      ( _xmlNode*    ptr )                    { Node_ptr::assign( ptr, ELEMENT_NODE ); }

    _xmlElement*                ptr                         () const                                { return (_xmlElement*)_ptr; }

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == ELEMENT_NODE || Node_ptr::is_type( type ); }

    string                      getAttribute                ( const char* name, const char* deflt = "" ) const;
    string                      getAttribute_mandatory      ( const string& name ) const;
    string                      getAttribute                ( const string& name, const string& deflt = "" ) const;
    bool                   bool_getAttribute                ( const string& name, bool deflt = false ) const;
    int                     int_getAttribute                ( const string& name ) const;
    int                     int_getAttribute                ( const string& name, int deflt ) const;
    int                    uint_getAttribute                ( const string& name, int deflt ) const;
    Attr_ptr                    getAttributeNode            ( const string& name ) const            { return getAttributeNode( name.c_str() ); }
    Attr_ptr                    getAttributeNode            ( const char*   name ) const;
    void                        setAttribute_optional       ( const string& name, const string& value ) const { if( value != "" )  setAttribute( name, value ); }
    void                        setAttribute                ( const string& name, const string& value ) const { setAttribute( name, value.c_str() ); }
    void                        setAttribute_optional       ( const string& name, const BSTR    value ) const { if( SysStringLen(value) > 0 )  setAttribute( name, value ); }
    void                        setAttribute                ( const string& name, const BSTR    value ) const { setAttribute( name, com::string_from_bstr( value ) ); }
    void                        setAttribute                ( const string& name, const char*   value ) const;
    void                        setAttribute                ( const string& name, bool value ) const;
    void                        setAttribute                ( const string& name, int64 value ) const;
    void                        setAttribute                ( const string& name, int value ) const           { setAttribute( name, (int64)value ); }  // Für gcc 3.3
    void                        setAttribute                ( const string& name, long value ) const          { setAttribute( name, (int64)value ); }  // Für gcc 3.3
    Attr_ptr                    setAttributeNode            ( _xmlAttr* newAttr ) const;
    bool                        hasAttributes               () const;
    bool                        removeAttribute             ( const string& name ) const;
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
};

//--------------------------------------------------------------------------------------Comment_ptr

struct Comment_ptr : CharacterData_ptr
{
                                Comment_ptr                 ( _xmlNode* p = NULL )                  : CharacterData_ptr(p) {}
                                Comment_ptr                 ( const Simple_node_ptr& p )            : CharacterData_ptr(p) {}

                                Comment_ptr                 ( const Comment_ptr& p )                : CharacterData_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == COMMENT_NODE || CharacterData_ptr::is_type( type ); }
    string                      data                        () const;
};

//---------------------------------------------------------------------------------CDATASection_ptr

struct CDATASection_ptr : Text_ptr
{
                                CDATASection_ptr            ( _xmlNode* p = NULL )                  : Text_ptr(p) {}
                                CDATASection_ptr            ( const Simple_node_ptr& p )            : Text_ptr(p) {}

                                CDATASection_ptr            ( const CDATASection_ptr& p )           : Text_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == CDATA_SECTION_NODE || Text_ptr::is_type( type ); }
};

//-------------------------------------------------------------------------------------------------

struct Xpath_nodes
{
    struct Xpath_object : Object, Non_cloneable
    {
                                Xpath_object                ()                                      : _xpath_context(NULL), _xpath_object(NULL) {}
                               ~Xpath_object                ();

        _xmlXPathContext*      _xpath_context;
        _xmlXPathObject*       _xpath_object;
    };

    int                         count                       () const;
    Simple_node_ptr             operator[]                  ( int index ) const                     { return get( index ); }
    Simple_node_ptr             get                         ( int index ) const;

    ptr<Xpath_object>          _xpath_object;
};

//------------------------------------------------------------------------ProcessingInstruction_ptr

struct ProcessingInstruction_ptr : Node_ptr 
{
                                ProcessingInstruction_ptr   ( _xmlNode* p = NULL )                  : Node_ptr(p) {}
                                ProcessingInstruction_ptr   ( const Simple_node_ptr& p )            : Node_ptr(p) {}
                                ProcessingInstruction_ptr   ( const ProcessingInstruction_ptr& p )  : Node_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == PROCESSING_INSTRUCTION_NODE || Node_ptr::is_type( type ); }

    string                      getTarget                   () const;
    string                      getData                     () const;
    void*                       setData                     ( const string& data );
};
//---------------------------------------------------------------------------------------Schema_ptr

struct Schema_ptr : Non_cloneable
{
                                Schema_ptr                  ()                                      : _ptr( NULL ) {}
                                Schema_ptr                  ( const Document_ptr& schema )          : _ptr( NULL ) { read( schema ); }
                               ~Schema_ptr                  ()                                      { release(); }

    void                        read                        ( const Document_ptr& schema );
    void                        release                     ();
    _xmlSchema**                operator &                  ()                                      { release();  return &_ptr; }
    bool                        operator !                  ()                                      { return _ptr == NULL; }
                                operator _xmlSchema*        ()                                      { return _ptr; }
    void                        validate                    ( const Document_ptr& );

    _xmlSchema*                _ptr;
};

//-------------------------------------------------------------------------------------------------

} //namespace xml_libxml2
} //namespace xml
} //namespace zschimmer

#endif
