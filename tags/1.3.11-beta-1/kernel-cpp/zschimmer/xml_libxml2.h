// $Id$

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

//---------------------------------------------------------------------------ImplementationRegistry
/*
struct ImplementationRegistry
{
    static Implementation*      getImplementation           ( const string& features );
    static void                 addSource                   ( ImplementationSource* source );
};
*/
//------------------------------------------------------------------------------Libxml2_thread_data
/*
struct Libxml2_thread_data
{
    string                     _error_text;
};
*/
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

//----------------------------------------------------------------------------------------Exception

struct Exception
{
    enum ExceptionCode
    {
        INDEX_SIZE_ERR              = 1,
        DOMSTRING_SIZE_ERR          = 2,
        HIERARCHY_REQUEST_ERR       = 3,
        WRONG_DOCUMENT_ERR          = 4,
        INVALID_CHARACTER_ERR       = 5,
        NO_DATA_ALLOWED_ERR         = 6,
        NO_MODIFICATION_ALLOWED_ERR = 7,
        NOT_FOUND_ERR               = 8,
        NOT_SUPPORTED_ERR           = 9,
        INUSE_ATTRIBUTE_ERR         = 10,
        INVALID_STATE_ERR           = 11,
        SYNTAX_ERR                  = 12,
        INVALID_MODIFICATION_ERR    = 13,
        NAMESPACE_ERR               = 14,
        INVALID_ACCESS_ERR          = 15,
        VALIDATION_ERR              = 16
    };

    ExceptionCode               code;
    string                      msg;
};

//-----------------------------------------------------------------------------ImplementationSource
/*
struct ImplementationSource
{
    Implementation*             getImplementation           ( const string& features ) const;
};
*/
//-----------------------------------------------------------------------------------Implementation
/*
struct Implementation
{
    bool                        hasFeature                  ( const string& feature, const string& version );
    ptr<DocumentType>           createDocumentType          ( const string& qualifiedName, const string& publicId, const string& systemId );
    ptr<Document>               createDocument              ( const string& namespaceURI, const string& qualifiedName, DocumentType* doctype );
    Implementation*             getInterface                ( const string& feature );
};
*/
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
    //enum { char_strict, char_weak } Character_handling;

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
  //void                    set_NodeValue                   ( const string& nodeValue );
    NodeType                    nodeType                    () const;
    Simple_node_ptr             parentNode                  () const;
    NodeList_ptr                childNodes                  () const;
    Simple_node_ptr             firstChild                  () const;
    Simple_node_ptr             lastChild                   () const;
    Simple_node_ptr             previousSibling             () const;
    Simple_node_ptr             nextSibling                 () const;
  //NamedNodeMap_ptr            attributes                  () const;
    Document_ptr                ownerDocument               () const;
    Simple_node_ptr             cloneNode                   ( bool deep ) const;
    Simple_node_ptr             insertBefore                ( const Simple_node_ptr& newChild, const Simple_node_ptr& refChild ) const;
    Simple_node_ptr             replaceChild                ( const Simple_node_ptr& newChild, const Simple_node_ptr& oldChild ) const;
    Simple_node_ptr             replace_with                ( const Simple_node_ptr& );
    void                        removeChild                 ( const Simple_node_ptr& child ) const;
    Simple_node_ptr             appendChild                 ( const Simple_node_ptr& newChild ) const;
    Simple_node_ptr             appendChild_if              ( const Simple_node_ptr& newChild ) const             { return newChild? appendChild( newChild ) : NULL; }
    bool                        hasChildNodes               () const;
    void                        normalize                   () const;
    bool                        isSupported                 ( const string& feature, const string& version ) const;
    string                      namespaceURI                () const;
    string                      prefix                      () const;
    string                      localName                   () const;
    void                        setPrefix                   ( const string& prefix ) const;
    bool                        isSameNode                  ( const Simple_node_ptr& other ) const;
    bool                        isEqualNode                 ( const Simple_node_ptr& arg ) const;
  //void*                       setUserData                 ( const string& key, void* data,  UserDataHandler* handler) const;
  //void*                       getUserData                 ( const string& key ) const;
    string                      getBaseURI                  () const;
    short                       compareTreePosition         ( const Simple_node_ptr& other ) const;
    string                      getTextContent              () const;
    void                        setTextContent              ( const string& textContent ) const;
    string                      lookupNamespacePrefix       ( const string& namespaceURI, bool useDefault ) const;
    bool                        isDefaultNamespace          ( const string& namespaceURI ) const;
    string                      lookupNamespaceURI          ( const string& prefix ) const;
    Simple_node_ptr             getInterface                ( const string& feature ) const;
    string                      xml_without_prolog          ( const string& encoding, bool indented = false ) const;
    string                      xml                         ( const string& encoding, bool indented = false ) const;
  //string                      xml                         ( bool indented = false ) const;
    string                      xml                         () const                                { return xml( default_character_encoding, false ); }
  //string                      ascii_xml                   () const                                { return xml( "ASCII", false ); }  // Liefert 7bit-Asci

    int                         line_number                 () const;
    Xpath_nodes                 select_nodes                ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node                 ( const string& xpath_expression ) const;
    bool                        has_node                    ( const string& xpath_expression ) const;
    Simple_node_ptr             select_node_strict          ( const string& xpath_expression ) const;
    Element_ptr                 select_element_strict       ( const string& xpath_expression ) const;


    _xmlNode*                   _ptr;
};

//-------------------------------------------------------------------------------------NamedNodeMap
/*
struct NamedNodeMap : Object
{
    Node_ptr                    setNamedItem                ( Node_ptr arg);
    Node_ptr                    item                        ( int index ) const;
    Node_ptr                    getNamedItem                ( const string& name) const;
    int                         getLength                   () const;
    Node_ptr                    removeNamedItem             ( const string& name);
    Node_ptr                    getNamedItemNS              ( const string& namespaceURI, const string& localName) const;
    Node_ptr                    setNamedItemNS              ( Node_ptr arg);
    Node_ptr                    removeNamedItemNS           ( const string& namespaceURI, const string& localName);
};
*/
//------------------------------------------------------------------------------------CharacterData
/*
struct CharacterData : Node 
{
    string                      getData                     () const;
    int                         getLength                   () const;
    string                      substringData               ( int offset, int count ) const;
    void                        appendData                  ( const string& arg );
    void                        insertData                  ( int offset, const string& arg );
    void                        deleteData                  ( int offset, int count );
    void                        replaceData                 ( int offset, int count, const string& arg );
    void                        setData                     ( const string& data );
};
*/
//-------------------------------------------------------------------------------------Document_ptr

struct Document_ptr : Simple_node_ptr, 
                      Libxml2_error_text
{
                                Document_ptr                ( _xmlDoc* doc = NULL )                 { assign( doc ); }
                                Document_ptr                ( const Document_ptr& doc )             { assign( doc.ptr() ); }
                              //Document_ptr                ( const Element_ptr& root_to_clone );
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
    void                        validate_against_dtd_string ( const string& dtd_string ) const;
    void                        validate_against_dtd        ( _xmlDtd* ) const;
  //void                        validate_against_schema_string ( const string& schema_string ) const;
  //void                        validate_against_schema     ( _xmlSchema* ) const;
    string                      xml                         ( const string& encoding, const string& indent_string = "" ) const;
  //string                      xml                         ( bool indented = false ) const         { return xml( "", indented ); }
    string                      xml                         () const                                { return xml( default_character_encoding ); }
  //string                      ascii_xml                   () const                                { return xml( "ASCII", false ); }  // Liefert 7bit-Asci

    Element_ptr                 createElement               ( const string& tagName ) const;
    Element_ptr                 createElement               ( const char* tagName ) const;
  //DocumentFragment_ptr        createDocumentFragment      ()                                      { return xmlNewDocFragment(); }
    Text_ptr                    createTextNode              ( const string& data ) const;
    Comment_ptr                 createComment               ( const string& data ) const;
    Comment_ptr                 createComment               ( const char* data ) const;
    CDATASection_ptr            createCDATASection          ( const string& data ) const;
    ProcessingInstruction_ptr   createProcessingInstruction ( const string& target, const string& data ) const;

    Simple_node_ptr             clone                       ( const Simple_node_ptr& node, int extended = 1 ) const;
    
  //ptr<Attr>                   createAttribute             ( const string& name ) const;
  //EntityReference*            createEntityReference       ( const string& name ) const;
    DocumentType_ptr            getDoctype                  () const;
  //Implementation*             implementation              () const;
    Element_ptr                 documentElement             () const;
  //NodeList*                   getElementsByTagName        ( const string& tagname) const;
  //Node_ptr                    importNode                  ( Node_ptr importedNode, bool deep );
  //Element_ptr                 createElementNS             ( const string& namespaceURI, const string& qualifiedName );
  //Attr*                       createAttributeNS           ( const string& namespaceURI, const string& qualifiedName );
    NodeList_ptr                getElementsByTagNameNS      ( const string& namespaceURI, const string& localName ) const;
  //Element_ptr                 getElementById              ( const string& elementId ) const;
    string                      getActualEncoding           () const;
    void                        setActualEncoding           ( const string& actualEncoding );
    string                      getEncoding                 () const;
    void                        setEncoding                 ( const string& encoding );
  //bool                        getStandalone               () const;
  //void                        setStandalone               ( bool standalone );
  //string                      getVersion                  () const;
  //void                        setVersion                  ( const string& version );
  //string                      getDocumentURI              () const;
  //void                        setDocumentURI              ( const string& documentURI );
  //bool                        getStrictErrorChecking      () const;
  //void                        setStrictErrorChecking      ( bool strictErrorChecking );
  //ErrorHandler*               getErrorHandler             () const;
  //void                        setErrorHandler             ( ErrorHandler* const handler );
  //Node_ptr                    renameNode                  ( Node_ptr n, const string& namespaceURI, const string& name );
  //Node_ptr                    adoptNode                   ( Node_ptr source );
  //void                        normalizeDocument           ();
  //bool                        canSetNormalizationFeature  ( const string& name, bool state ) const;
  //void                        setNormalizationFeature     ( const string& name, bool state);
  //bool                        getNormalizationFeature     ( const string& name) const;

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


  //Node_ptr                    item                        ( int index ) const                     { return _ptr->item[ index ]; }
  //int                         length                      () const                                { return _ptr->length; }
};

//---------------------------------------------------------------------------------DocumentFragment 
/*
struct DocumentFragment : Node 
{
};
*/
//--------------------------------------------------------------------------------CharacterData_ptr

struct CharacterData_ptr : Node_ptr 
{
                                CharacterData_ptr           ( _xmlNode* p = NULL )                  : Node_ptr(p) {}
                                CharacterData_ptr           ( const Simple_node_ptr& p )            : Node_ptr(p) {}
                                CharacterData_ptr           ( const CharacterData_ptr& p )          : Node_ptr(p) {}

    string                      getData                     () const;
    int                         getLength                   () const;
    string                      substringData               ( int offset, int count ) const;
    void                        appendData                  ( const string& arg );
    void                        insertData                  ( int offset, const string& arg );
    void                        deleteData                  ( int offset, int count );
    void                        replaceData                 ( int offset, int count, const string& arg );
    void                        setData                     ( const string& data );
};

//-----------------------------------------------------------------------------------------Text_ptr

struct Text_ptr : CharacterData_ptr
{
                                Text_ptr                    ( _xmlNode* p = NULL )                  : CharacterData_ptr(p) {}
                                Text_ptr                    ( const Simple_node_ptr& p )            : CharacterData_ptr(p) {}

                                Text_ptr                    ( const Text_ptr& p )                   : CharacterData_ptr(p) {}

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == TEXT_NODE || CharacterData_ptr::is_type( type ); }

  //string                      data                        () const                                { return sd_free( xmlNodeListGetString( _ptr->doc, _ptr, 1 ) ); }
    string                      data                        () const;
  //ptr<Text>                   splitText                   ( int offset );
  //bool                        getIsWhitespaceInElementContent() const;
  //string                      getWholeText                ();
  //ptr<Text>                   replaceWholeText            ( const string& content);
};

//-----------------------------------------------------------------------------------------Attr_ptr

struct Attr_ptr : Node_ptr
{
                                Attr_ptr                    ( _xmlAttr* ptr )                       : Node_ptr( (_xmlNode*)ptr ) {} 

    virtual bool                is_type                     ( NodeType type )                       { return nodeType() == ATTRIBUTE_NODE || Attr_ptr::is_type( type ); }

    string                      name                        ();
    bool                        specified                   ();
  //string                      value                       ()                                      { return sd_free( xmlNodeListGetString( _ptr->doc, _ptr->children, 1 ) ); }
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

  //Element_ptr&                operator =                  ( _xmlElement* ptr )                    { assign( ptr ); }
  //Element_ptr&                operator =                  ( _xmlNode*    ptr )                    { assign( ptr ); }

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
  //time_t               time_t_getAttribute                ( const string& name, time_t deflt ) const;
    Attr_ptr                    getAttributeNode            ( const string& name ) const            { return getAttributeNode( name.c_str() ); }
    Attr_ptr                    getAttributeNode            ( const char*   name ) const;
  //NodeList*                   getElementsByTagName        ( const string& name );  
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
  //Attr*                       removeAttributeNode         ( _xmlAttr* oldAttr ) const;
    bool                        removeAttribute             ( const string& name ) const;
  //string                      getAttributeNS              ( const string& namespaceURI, const string& localName ) const;
  //void                        setAttributeNS              ( const string& namespaceURI, const string& qualifiedName, const string& value );
  //void                        removeAttributeNS           ( const string& namespaceURI, const string& localName );
  //Attr*                       getAttributeNodeNS          ( const string& namespaceURI, const string& localName ) const;
  //Attr*                       setAttributeNodeNS          ( Attr* newAttr );
  //NodeList*                   getElementsByTagNameNS      ( const string& namespaceURI, const string& localName ) const;
    bool                        hasAttribute                ( const string& name ) const;
  //bool                        hasAttributeNS              ( const string& namespaceURI, const string& localName ) const;
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

  //string                      xml                         ( bool indented = false ) const;
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

//----------------------------------------------------------------------------------UserDataHandler
/*
struct UserDataHandler 
{
    enum OperationType 
    {
        NODE_CLONED   = 1,
        NODE_IMPORTED = 2,
        NODE_DELETED  = 3,
        NODE_RENAMED  = 4
    };

    virtual void                handle                      ( OperationType operation, const string& key, void* data, const Node_ptr src, const Node_ptr dst ) = 0;
};
*/
//--------------------------------------------------------------------------------------------Error
/*
struct Error 
{
    enum ErrorSeverity 
    {
        _SEVERITY_WARNING     = 0,
        _SEVERITY_ERROR       = 1,
        _SEVERITY_FATAL_ERROR = 2
    };

    short                       getSeverity                 () const;
    string                      getMessage                  () const;
    Locator*                    getLocation                 () const;
    void*                       getRelatedException         () const;
    void                        setSeverity                 ( ErrorSeverity severity );
    void                        setMessage                  ( const string& message );
    void                        setLocation                 ( Locator* const location );
    void                        setRelatedException         ( void* exception) const;
};
*/
//-------------------------------------------------------------------------------------ErrorHandler
/*
struct ErrorHandler 
{
    virtual bool                handleError                 ( const Error& domError ) = 0;
};
*/
//------------------------------------------------------------------------------------------Locator
/*
struct Locator 
{
    int                         getLineNumber               () const;
    int                         getColumnNumber             () const;
    int                         getOffset                   () const;
    Node_ptr                    getErrorNode                () const;
    string                      getURI                      () const;
    void                        setLineNumber               ( int lineNumber );
    void                        setColumnNumber             ( int columnNumber );
    void                        setOffset                   ( int offset );
    void                        setErrorNode                ( Node_ptr const errorNode );
    void                        setURI                      ( const string& uri );
};
*/
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


/*
    struct iterator
    {
                                iterator                    ( Xpath_nodes* x, int index = 0 )       : _xpath_nodes(x), _index(index) {}


        bool                    operator ==                 ( int index )                           { return _index == index; }
        Node_ptr                operator *                  () const                                { return _xpath_nodes[ _index ]; ]
        Node_ptr                operator ->                 () const                                { return _xpath_nodes[ _index ]; ]

        int                    _index;
        ptr<Xpath_nodes>       _xpath_nodes;
    };
*/

  //iterator                    begin                       () const                                { return iterator( this ); }
  //int                         end                         () const                                { return count(); }
    int                         count                       () const;
    Simple_node_ptr             operator[]                  ( int index ) const                     { return get( index ); }
    Simple_node_ptr             get                         ( int index ) const;

    ptr<Xpath_object>          _xpath_object;
};

//-------------------------------------------------------------------------------------DocumentType
/*
struct DocumentType : Node 
{
    string                      getName                     () const;
    NamedNodeMap*               getEntities                 () const;
    NamedNodeMap*               getNotations                () const;
    string                      getPublicId                 () const;
    string                      getSystemId                 () const;
    string                      getInternalSubset           () const;
};
*/
//-----------------------------------------------------------------------------------------Notation
/*
struct Notation : Node 
{
    string                      getPublicId                 () const;
    string                      getSystemId                 () const;
};
*/
//-------------------------------------------------------------------------------------------Entity
/*
struct Entity : Node 
{
    string                      getPublicId                 () const;
    string                      getSystemId                 () const;
    string                      getNotationName             () const;
    string                      getActualEncoding           () const;
    void                        setActualEncoding           ( const string& actualEncoding );
    string                      getEncoding                 () const;
    void                        setEncoding                 ( const string& encoding );
    string                      getVersion                  () const;
    void                        setVersion                  ( const string& version );
};
*/
//----------------------------------------------------------------------------------EntityReference
/*
struct EntityReference : Node 
{
};
*/
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

//------------------------------------------------------------------------------------------Dtd_ptr

struct Dtd_ptr : Non_cloneable
{   
                                Dtd_ptr                     ()                                      : _ptr( NULL ) {}
                                Dtd_ptr                     ( const string& dtd )                   : _ptr( NULL ) { read( dtd ); }
                               ~Dtd_ptr                     ()                                      { release(); }

    void                        read                        ( const string& dtd_string );
    void                        release                     ();
    _xmlDtd**                   operator &                  ()                                      { release();  return &_ptr; }
    bool                        operator !                  ()                                      { return _ptr == NULL; }
                                operator _xmlDtd*           ()                                      { return _ptr; }

    Document_ptr                validate_xml                ( const string& xml )                   { Document_ptr result ( xml );
                                                                                                      result.validate_against_dtd( _ptr );
                                                                                                      return result; }

    _xmlDtd*                    _ptr;
};

//---------------------------------------------------------------------------------------Schema_ptr

struct Schema_ptr : Non_cloneable
{   
                                Schema_ptr                  ()                                      : _ptr( NULL ) {}
                                Schema_ptr                  ( const Document_ptr& schema )          : _ptr( NULL ) { read( schema ); }
                              //Schema_ptr                  ( const string& schema )                : _ptr( NULL ) { read( schema ); }
                               ~Schema_ptr                  ()                                      { release(); }

    void                        read                        ( const Document_ptr& schema );
  //void                        read                        ( const string& schema_string );
    void                        release                     ();
    _xmlSchema**                operator &                  ()                                      { release();  return &_ptr; }
    bool                        operator !                  ()                                      { return _ptr == NULL; }
                                operator _xmlSchema*        ()                                      { return _ptr; }

    void                        validate                    ( const Document_ptr& );
  //Document_ptr                validate_xml                ( const string& xml )                   { Document_ptr result ( xml );
  //                                                                                                  result.validate_against_schema( _ptr );
  //                                                                                                  return result; }

    _xmlSchema*                _ptr;
};

//---------------------------------------------------------------------------------ImplementationLS
/* ?
struct ImplementationLS 
{
    enum Mode
    {
        MODE_SYNCHRONOUS =1,
        MODE_SYNCHRONOUS =2
    };

    Builder*                    createBuilder               ( Mode mode, const string& schemaType );
    Writer*                     createWriter                ();
    InputSource*                createInputSource           ();
};
*/
//--------------------------------------------------------------------------------------InputSource
/*
struct InputSource 
{
    string                      getEncoding                 () const;
    string                      getPublicId                 () const;
    string                      getSystemId                 () const;
    string                      getBaseURI                  () const;
    string                      setEncoding                 () const;
    string                      setPublicId                 () const;
    string                      setSystemId                 () const;
    string                      setBaseURI                  () const;
    //
    // Called to indicate that this InputSource is no longer in use
    //   and that the implementation may relinquish any resources associated with it.
    //
    // Access to a released object will lead to unexpected result.
    //
    void                        release                     ();
};
*/
//-----------------------------------------------------------------------------------EntityResolver
/*
struct EntityResolver 
{
    InputSource*                resolverEntity              ( const string& publicId, const string& systemId,  const string& baseURI );
};
*/
//------------------------------------------------------------------------------------------Builder
/*
struct Builder 
{
    enum ActionType 
    {
        ACTION_REPLACE              = 1,
        ACTION_APPEND_AS_CHILDREN   = 2,
        ACTION_INSERT_AFTER         = 3,
        ACTION_INSERT_BEFORE        = 4
    };

    ErrorHandler*               getErrorHandler             ();
    const ErrorHandler*         getErrorHandler             () const;
    EntityResolver*             getEntityResolver           ();
    const EntityResolver*       getEntityResolver           () const;
    BuilderFilter*              getFilter                   ();
    const BuilderFilter*        getFilter                   () const;
    void                        setErrorHandler             ( ErrorHandler* const handler );
    void                        setEntityResolver           ( EntityResolver* const handler );
    void                        setFilter                   ( BuilderFilter* const filter );
    void                        setFeature                  ( const string& name, const bool state );
    bool                        getFeature                  ( const string& name ) const;
    bool                        canSetFeature               ( const string& name, const bool state ) const;
    Document*                   parse                       ( const InputSource& source );
    Document*                   parseURI                    ( const string& systemId );
    void                        parseWithContext            ( const InputSource& source, Node_ptr const contextNode, ActionType action );
    //
    // Called to indicate that this Builder is no longer in use
    //   and that the implementation may relinquish any resources associated with it.
    //
    // Access to a released object will lead to unexpected result.
    //
    void                        release                     ();
};
*/
//-------------------------------------------------------------------------------------------Writer
// The XMLFormatTarget is implementation specific
/*
struct Writer 
{
    bool                        canSetFeature               ( const string& featName, bool state ) const;
    void                        setFeature                  ( const string& featName, bool state );
    bool                        getFeature                  ( const string& featName ) const;
    void                        setEncoding                 ( const string& encoding );
    void                        setNewLine                  ( const string& newLine );
    void                        setErrorHandler             ( ErrorHandler* errorHandler );
    void                        setFilter                   ( WriterFilter* filter );
    string                      getEncoding                 () const;
    string                      getNewLine                  () const;
    ErrorHandler*               getErrorHandler             () const;
    WriterFilter*               getFilter                   () const;
    bool                        writeToNode                 ( XMLFormatTarget* const destination, const Node& nodeToWrite );
    string                      writeToString               ( const Node& nodeToWrite );
    //
    // Called to indicate that this Writer is no longer in use
    //   and that the implementation may relinquish any resources associated with it.
    //
    // Access to a released object will lead to unexpected result.
    //
    void                        release();
};
*/
//-------------------------------------------------------------------------------------WriterFilter
/*
struct WriterFilter : NodeFilter 
{
    unsigned long               getWhatToShow               () const;
    void                        setWhatToShow               ( unsigned long toShow );
};
*/

//------------------------------------------------------------------------Element_ptr_with_document
// Um ein Element_ptr aufzuheben, dass nur von einem Document_ptr gehalten wird
// (mit meiner Änderung des Codes von libxml2, siehe Kommentar "zschimmer").

//struct Element_ptr_with_document : Element_ptr
//{
//                                Element_ptr_with_document   ( const Document_ptr& d, const Element_ptr& e ) : Element_ptr(e), _document(d) {}
//
//    Document_ptr               _document;
//};

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

} //namespace xml_libxml2
} //namespace xml
} //namespace zschimmer

#endif
