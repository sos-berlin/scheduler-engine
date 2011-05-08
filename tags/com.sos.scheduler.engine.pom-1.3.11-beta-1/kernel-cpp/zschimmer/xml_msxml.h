// $Id$

#ifndef __ZSCHIMMER_XML_MSXML_H
#define __ZSCHIMMER_XML_MSXML_H

/*
    Implementierung ähnlich http://www.w3.org/TR/2002/WD-DOM-Level-3-Core-20021022/core.html
*/

#include "xml.h"
#include "z_com.h"


//#import <msxml3.dll> rename_namespace("msxml")
#import <msxml.tlb> rename_namespace("msxml")


namespace msxml
{
    typedef IXMLDOMAttribute        Attr;
    typedef IXMLDOMAttributePtr     Attr_ptr;
    typedef IXMLDOMElement          Element;
    typedef IXMLDOMElementPtr       Element_ptr;
    typedef IXMLDOMCharacterData    CharacterData;
    typedef IXMLDOMCharacterDataPtr CharacterData_ptr;
    typedef IXMLDOMComment          Comment;
    typedef IXMLDOMCommentPtr       Comment_ptr;
    typedef IXMLDOMText             Text;
    typedef IXMLDOMTextPtr          Text_ptr;
    typedef IXMLDOMCDATASection     CDATASection;
    typedef IXMLDOMCDATASectionPtr  CDATASection_ptr;
    typedef IXMLDOMNode             Node;
    typedef IXMLDOMNodePtr          Node_ptr;
    typedef IXMLDOMNodeList         NodeList;
    typedef IXMLDOMNodeListPtr      NodeList_ptr;
    typedef IXMLDOMDocument         Document;
    typedef IXMLDOMDocumentPtr      Document_ptr;
    typedef IXMLDOMDocumentTypePtr  DocumentType_ptr;
    typedef IXMLDOMProcessingInstruction ProcessingInstruction;
    typedef IXMLDOMProcessingInstructionPtr ProcessingInstruction_ptr;
}



namespace zschimmer {
namespace xml_msxml {
namespace xml {


typedef _bstr_t DOMString;


struct Attr_ptr;
struct CDATASection_ptr;
struct Comment_ptr;
struct Document_ptr;
struct DocumentFragment_ptr;
struct DocumentType_ptr;
struct Element_ptr;
//struct ErrorHandler_ptr;
//struct Implementation_ptr;
//struct ImplementationSource_ptr;
struct NodeList_ptr;
struct NamedNodeMap_ptr;        
//struct UserDataHandler_ptr;
//struct Locator_ptr;
struct ProcessingInstruction_ptr;
struct Text_ptr;
//struct EntityReference_ptr;
struct InputSource_ptr;
//struct BuilderFilter_ptr;
//struct WriterFilter_ptr;
//struct XMLFormatTarget_ptr;


//----------------------------------------------------------------------------string_from_DOMString

inline string string_from_DOMString( const DOMString& s )
{
    return zschimmer::com::string_from_bstr( s );
}

//----------------------------------------------------------------------------DOMString_from_string

inline _bstr_t DOMString_from_string( const string& s )
{
    return _bstr_t( s.c_str() );
}

//---------------------------------------------------------------------------ImplementationRegistry
/*
struct ImplementationRegistry
{
    static Implementation*      getImplementation           ( const string& features );
    static void                 addSource                   ( ImplementationSource* source );
};
*/
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
    ELEMENT_NODE                    = 1,
    ATTRIBUTE_NODE                  = 2,
    TEXT_NODE                       = 3,
    CDATA_SECTION_NODE              = 4,
    ENTITY_REFERENCE_NODE           = 5,
    ENTITY_NODE                     = 6,
    PROCESSING_INSTRUCTION_NODE     = 7,
    COMMENT_NODE                    = 8,
    DOCUMENT_NODE                   = 9,
    DOCUMENT_TYPE_NODE              = 10,
    DOCUMENT_FRAGMENT_NODE          = 11,
    NOTATION_NODE                   = 12
};

//-----------------------------------------------------------------------------------------Node_ptr

struct Node_ptr 
{
                                Node_ptr                    ()                                      : _ptr(NULL) {}

                                template< class T >
                                Node_ptr                    ( T* p )                                : _ptr(p) {}

                                template< class T >
                                Node_ptr                    ( const _com_ptr_t<T>& ptr )            : _ptr(ptr) {}

                                Node_ptr                    ( const Node_ptr& p )                   : _ptr(p._ptr) {}

    msxml::Node*                ptr                         ()                                      { return _ptr; }

                                operator msxml::Node*       ()                                      { return ptr(); }

    string                      nodeName                    () const                                { return string_from_DOMString( _ptr->nodeName ); }
  //void                    set_nodeName                    ( const string& name )                  { _ptr->nodeName = DOMString_from_string(name); }
    bool                        nodeName_is                 ( const char* name ) const;
    bool                        nodeName_is                 ( const string& name ) const            { return nodeName_is( name.c_str() ); }
    string                      nodeValue                   () const                                { return zschimmer::com::string_from_variant( _ptr->nodeValue ); }
  //void                    set_NodeValue                   ( const string& nodeValue );
    NodeType                    nodeType                    () const                                { return (NodeType) _ptr->nodeType; }
    Node_ptr                    parentNode                  () const;
    NodeList_ptr                childNodes                  () const;
    Node_ptr                    firstChild                  () const                                { return _ptr->firstChild; }
    Node_ptr                    lastChild                   () const                                { return _ptr->lastChild; }
    Node_ptr                    previousSibling             () const                                { return _ptr->previousSibling; }
    Node_ptr                    nextSibling                 () const                                { return _ptr->nextSibling; }
  //NamedNodeMap_ptr            attributes                  () const;
    Document_ptr                ownerDocument               () const;
  //Node_ptr                    cloneNode                   ( bool deep );
  //Node_ptr                    insertBefore                ( Node_ptr newChild, Node_ptr refChild );
  //Node_ptr                    replaceChild                ( Node_ptr newChild, Node_ptr oldChild );
  //Node_ptr                    removeChild                 ( Node_ptr oldChild );
    Node_ptr                    appendChild                 ( Node_ptr newChild ) const             { return _ptr->appendChild( newChild ); }
    bool                        hasChildNodes               ()                                      { return _ptr->firstChild != NULL; }
    void                        normalize                   ();
    bool                        isSupported                 ( const string& feature, const string& version );
    string                      namespaceURI                ();
    string                      prefix                      ();
    string                      localName                   ();
    void                        setPrefix                   ( const string& prefix );
    bool                        hasAttributes               ();
    bool                        isSameNode                  ( const Node_ptr other );
    bool                        isEqualNode                 ( const Node_ptr arg );
  //void*                       setUserData                 ( const string& key, void* data,  UserDataHandler* handler);
  //void*                       getUserData                 ( const string& key );
    string                      getBaseURI                  ();
    short                       compareTreePosition         ( Node_ptr other );
    string                      getTextContent              () const                                { return string_from_DOMString( _ptr->text ); }
    void                        setTextContent              ( const string& textContent );
    string                      lookupNamespacePrefix       ( const string& namespaceURI, bool useDefault );
    bool                        isDefaultNamespace          ( const string& namespaceURI );
    string                      lookupNamespaceURI          ( const string& prefix );
  //Node_ptr                    getInterface                ( const string& feature );

  //zschimmer::ptr<msxml::Node> _ptr;
    msxml::Node_ptr            _ptr;
};

//-------------------------------------------------------------------------------------NodeList_ptr

struct NodeList_ptr : Node_ptr
{
                                NodeList_ptr                ( msxml::NodeList* p )                  :  Node_ptr( p ), _ptr(p) {}

    Node_ptr                    item                        ( int index ) const                     { return _ptr->item[ index ]; }
    int                         length                      () const                                { return _ptr->length; }

    msxml::NodeList_ptr        _ptr; 
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
//--------------------------------------------------------------------------------CharacterData_ptr

struct CharacterData_ptr : Node_ptr 
{
                                CharacterData_ptr           ( const Node_ptr& e )                   : Node_ptr(e), _ptr(e._ptr) {}
                                CharacterData_ptr           ( msxml::CharacterData* e = NULL )      : Node_ptr(e), _ptr(e) {}

                                template<class T>
                                CharacterData_ptr           ( const _com_ptr_t<T>& p )              : Node_ptr(p), _ptr(p) {}

                                CharacterData_ptr           ( const CharacterData_ptr& e )          : Node_ptr(e), _ptr(e._ptr) {}

    string                      getData                     () const;
    int                         getLength                   () const;
    string                      substringData               ( int offset, int count ) const;
    void                        appendData                  ( const string& arg );
    void                        insertData                  ( int offset, const string& arg );
    void                        deleteData                  ( int offset, int count );
    void                        replaceData                 ( int offset, int count, const string& arg );
    void                        setData                     ( const string& data );

    msxml::CharacterData_ptr   _ptr;
};

//-------------------------------------------------------------------------------------Document_ptr

struct Document_ptr : Node_ptr//, Non_cloneable
{
                                Document_ptr                ()                                      : _ptr(NULL) {}

                                Document_ptr                ( msxml::Document* ptr )                : Node_ptr(ptr), _ptr(ptr) {}

                                template< class T >
                                Document_ptr                ( const _com_ptr_t<T>& ptr )            : Node_ptr(ptr), _ptr(ptr) {}

                                Document_ptr                ( const Node_ptr& ptr )                 : Node_ptr(ptr), _ptr(ptr._ptr) {}


    msxml::Document*            ptr                         ()                                      { return _ptr; }
    
    void                        set_reference               ( const Document_ptr& doc )             { _ptr = doc._ptr; }
    msxml::Document_ptr         detach                      ()                                      { msxml::Document_ptr result = _ptr; _ptr = NULL; Node_ptr::_ptr = NULL; return result; }

    void                        create                      ();

    bool                        try_load_xml                ( const string& xml_text )              { return _ptr->loadXML( _bstr_t( zschimmer::com::bstr_from_string(xml_text), false ) ) != 0; }
    void                        load_xml                    ( const string& xml_text );
    string                      error_text                  ();
    string                      xml                         ( bool indented = false ) const;

    Element_ptr                 createElement               ( const string& tagName ) const;
  //DocumentFragment            createDocumentFragment      () const                                { return xmlNewDocFragment(); }
    Text_ptr                    createTextNode              ( const string& data ) const;
    Comment_ptr                 createComment               ( const string& data ) const;
    CDATASection_ptr            createCDATASection          ( const string& data ) const;
    ProcessingInstruction_ptr   createProcessingInstruction ( const string& target, const string& data ) const;
  //ptr<Attr>                   createAttribute             ( const string& name ) const;
  //EntityReference*            createEntityReference       ( const string& name ) const;
    DocumentType_ptr            getDoctype                  () const;
  //Implementation*             implementation              () const;
    Element_ptr                 documentElement             () const;
  //NodeList_ptr                getElementsByTagName        ( const string& tagname) const;
  //Node_ptr                    importNode                  ( Node_ptr importedNode, bool deep );
  //Element_ptr                 createElementNS             ( const string& namespaceURI, const string& qualifiedName );
  //Attr*                       createAttributeNS           ( const string& namespaceURI, const string& qualifiedName );
  //NodeList_ptr                getElementsByTagNameNS      ( const string& namespaceURI, const string& localName ) const;
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

    msxml::Document_ptr        _ptr;
};                                                          
                                                            
//---------------------------------------------------------------------------------DocumentFragment 
/*
struct DocumentFragment : Node_ptr 
{
};
*/
//-----------------------------------------------------------------------------------------Attr_ptr

struct Attr_ptr : Node_ptr
{
                                Attr_ptr                    ( const Node_ptr& e )                   : Node_ptr(e), _ptr(e._ptr) {}
                                Attr_ptr                    ( msxml::Attr* e = NULL )               : Node_ptr(e), _ptr(e) {}

                                template<class T>
                                Attr_ptr                    ( const _com_ptr_t<T>& p )              : Node_ptr(p), _ptr(p) {}

                                Attr_ptr                    ( const Attr_ptr& e )                   : Node_ptr(e), _ptr(e._ptr) {}

    string                      name                        ()                                      { return string_from_DOMString( _ptr->name ); }
    bool                        specified                   ()                                      { return _ptr->specified != 0; }
    string                      value                       ()                                      { return zschimmer::com::string_from_variant( _ptr->value ); }
    void                    set_value                       ( const string& value )                 { _ptr->value = value.c_str(); }
  //Element_ptr                 ownerElement                ();

    msxml::Attr_ptr            _ptr;
};

//--------------------------------------------------------------------------------------Element_ptr

struct Element_ptr : Node_ptr
{
    enum No_xc { no_xc };


                                Element_ptr                 ()                                      : _ptr(NULL) {}
                                Element_ptr                 ( const Node_ptr& e )                   : Node_ptr(e), _ptr(e._ptr) {}
                                Element_ptr                 ( const Node_ptr& e, No_xc )            : Node_ptr(e), _ptr(e._ptr) {}
                                Element_ptr                 ( msxml::Element* e )                   : Node_ptr(e), _ptr(e) {}

                                template<class T>
                                Element_ptr                 ( const _com_ptr_t<T>& p )              : Node_ptr(p), _ptr(p) {}       // Hier sollte bei falschem Typ Exception ausgelöst werden
                                
                                template<class T>
                                Element_ptr                 ( const _com_ptr_t<T>& p, No_xc )       : Node_ptr(p), _ptr(p) {}
                                
                                Element_ptr                 ( const Element_ptr& e )                : Node_ptr(e), _ptr(e._ptr) {}


    msxml::Element*             ptr                         ()                                      { return _ptr; }

    string                      getAttribute                ( const string& name, const string& deflt = "" ) const  { Attr_ptr a = getAttributeNode( name ); return a? a.value() : deflt; }
    string                      getAttribute                ( const char*   name, const string& deflt = "" ) const  { Attr_ptr a = getAttributeNode( name ); return a? a.value() : deflt; }

    bool                   bool_getAttribute                ( const string& name, bool deflt = false ) const;
    int                     int_getAttribute                ( const string& name, int deflt = 0 ) const;
    int                    uint_getAttribute                ( const string& name, int deflt = 0 ) const;
    
    Attr_ptr                    getAttributeNode            ( const string& name ) const            { return _ptr->getAttributeNode( name.c_str() ); }
    Attr_ptr                    getAttributeNode            ( const char*   name ) const            { return _ptr->getAttributeNode( name         ); }
    NodeList_ptr                getElementsByTagName        ( const string& name ) const            { return (NodeList_ptr)_ptr->getElementsByTagName( name.c_str() ); }
    NodeList_ptr                getElementsByTagName        ( const char*   name ) const            { return (NodeList_ptr)_ptr->getElementsByTagName( name         ); }
    void                        setAttribute                ( const string& name, const string& value ) const   { setAttribute( name.c_str(), value ); }
    void                        setAttribute                ( const char*   name, const string& value ) const;
    void                        setAttribute                ( const string& name, int value ) const;
  //Attr                        setAttributeNode            ( xmlAttr* newAttr ) const;
  //Attr*                       removeAttributeNode         ( xmlAttr* oldAttr );
  //void                        removeAttribute             ( const string& name ) const;
  //string                      getAttributeNS              ( const string& namespaceURI, const string& localName ) const;
  //void                        setAttributeNS              ( const string& namespaceURI, const string& qualifiedName, const string& value ) const;
  //void                        removeAttributeNS           ( const string& namespaceURI, const string& localName ) const;
  //Attr*                       getAttributeNodeNS          ( const string& namespaceURI, const string& localName ) const;
  //Attr*                       setAttributeNodeNS          ( Attr* newAttr ) const;
  //NodeList*                   getElementsByTagNameNS      ( const string& namespaceURI, const string& localName ) const;
  //bool                        hasAttribute                ( const string& name ) const;
  //bool                        hasAttributeNS              ( const string& namespaceURI, const string& localName ) const;

    msxml::Element_ptr         _ptr;
};

//-----------------------------------------------------------------------------------------Text_ptr

struct Text_ptr : CharacterData_ptr
{
                                Text_ptr                    ( const Node_ptr& e )                   : CharacterData_ptr(e), _ptr(e._ptr) {}
                                Text_ptr                    ( msxml::Text* e = NULL )               : CharacterData_ptr(e), _ptr(e) {}

                                template<class T>
                                Text_ptr                    ( const _com_ptr_t<T>& p )              : CharacterData_ptr(p), _ptr(p) {}

                                Text_ptr                    ( const Text_ptr& e )                   : CharacterData_ptr(e), _ptr(e._ptr) {}


    string                      data                        () const                                { return string_from_DOMString( _ptr->data ); }
  //ptr<Text>                   splitText                   ( int offset );
  //bool                        getIsWhitespaceInElementContent() const;
  //string                      getWholeText                ();
  //ptr<Text>                   replaceWholeText            ( const string& content);

    msxml::Text_ptr            _ptr;
};

//--------------------------------------------------------------------------------------Comment_ptr

struct Comment_ptr : CharacterData_ptr
{
                                Comment_ptr                 ( const Node_ptr& e )                   : CharacterData_ptr(e), _ptr(e._ptr) {}
                                Comment_ptr                 ( msxml::Comment_ptr* e = NULL )        : CharacterData_ptr(e), _ptr(e) {}

                                template<class T>
                                Comment_ptr                 ( const _com_ptr_t<T>& p )              : CharacterData_ptr(p), _ptr(p) {}

                                Comment_ptr                 ( const Comment_ptr& e )                : CharacterData_ptr(e), _ptr(e._ptr) {}

    msxml::Comment_ptr         _ptr;
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
                                CDATASection_ptr            ( const Node_ptr& e )                   : Text_ptr(e), _ptr(e._ptr) {}
                                CDATASection_ptr            ( msxml::CDATASection* e = NULL )       : Text_ptr(e), _ptr(e) {}

                                template<class T>
                                CDATASection_ptr            ( const _com_ptr_t<T>& p )              : Text_ptr(p), _ptr(p) {}

                                CDATASection_ptr            ( const CDATASection_ptr& e )           : Text_ptr(e), _ptr(e._ptr) {}

    msxml::CDATASection_ptr    _ptr;
};

//-------------------------------------------------------------------------------------DocumentType
/*
struct DocumentType : Node_ptr 
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
struct Notation : Node_ptr 
{
    string                      getPublicId                 () const;
    string                      getSystemId                 () const;
};

//-------------------------------------------------------------------------------------------Entity
/*
struct Entity : Node_ptr 
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
struct EntityReference : Node_ptr 
{
};
*/
//----------------------------------------------------------------------------ProcessingInstruction

struct ProcessingInstruction_ptr : Node_ptr 
{
                                ProcessingInstruction_ptr   ( const Node_ptr& e )                   : Node_ptr(e), _ptr(e._ptr) {}
                                ProcessingInstruction_ptr   ( msxml::ProcessingInstruction* e = NULL ) : Node_ptr(e), _ptr(e) {}

                                template<class T>
                                ProcessingInstruction_ptr   ( const _com_ptr_t<T>& p )              : Node_ptr(p), _ptr(p) {}

                                ProcessingInstruction_ptr   ( const ProcessingInstruction_ptr& e )  : Node_ptr(e), _ptr(e._ptr) {}

    string                      getTarget                   () const;
    string                      getData                     () const;
    void*                       setData                     ( const string& data );

    msxml::ProcessingInstruction_ptr _ptr;
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
    bool                        writeToNode                 ( XMLFormatTarget* const destination, const Node_ptr& nodeToWrite );
    string                      writeToString               ( const Node_ptr& nodeToWrite );
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
//-------------------------------------------------------------------------------------------------

//inline Element_ptr      Attr_ptr::ownerElement              ()                                      { return _ptr->ownerElement; }
inline NodeList_ptr     Node_ptr::childNodes                () const                                { return NodeList_ptr( _ptr->childNodes ); }
inline Document_ptr     Node_ptr::ownerDocument             () const                                { return _ptr->ownerDocument; }
inline Element_ptr      Document_ptr::createElement         ( const string& tagName ) const         { return _ptr->createElement( DOMString_from_string(tagName) ); }
inline Text_ptr         Document_ptr::createTextNode        ( const string& data ) const            { return _ptr->createTextNode( DOMString_from_string(data) ); }
inline Comment_ptr      Document_ptr::createComment         ( const string& data ) const            { return _ptr->createComment( DOMString_from_string(data) ); }
inline CDATASection_ptr Document_ptr::createCDATASection    ( const string& data ) const            { return _ptr->createCDATASection( DOMString_from_string(data) ); }
inline ProcessingInstruction_ptr Document_ptr::createProcessingInstruction( const string& target, const string& data ) const  { return _ptr->createProcessingInstruction( target.c_str(), data.c_str() ); }
inline Element_ptr      Document_ptr::documentElement       () const                                { return _ptr->documentElement; }

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace xml_msxml
} //namespace zschimmer

#endif
