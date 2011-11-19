// $Id: xml_sax.cxx 11394 2005-04-03 08:30:29Z jz $       

#include "precomp.h"
#include "sos.h"
#include "xml_sax.h"

#ifdef SYSTEM_WIN

#include "olestd.h"
#include <windows.h>
#include <ole2.h>

//#   include "../misc/microsoft/msxml/msxml2.h"
#import <msxml3.dll> raw_interfaces_only 
using namespace MSXML2;

namespace sos {
namespace xml {
namespace sax {

//Ole_class_descr* sax_content_handler_for_com_class_ptr;

//---------------------------------------------------------------------------Content_handler_for_com

struct Content_handler_for_com: MSXML2::ISAXContentHandler, Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware.Content_handler_for_com" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Content_handler_for_com     () : Sos_ole_object( NULL, this, NULL ) {}
    virtual                    ~Content_handler_for_com     () {}

    USE_SOS_OLE_OBJECT
        
    HRESULT STDMETHODCALLTYPE   startElement                ( wchar_t*, int, wchar_t*, int, wchar_t*, int, ISAXAttributes* );
    HRESULT STDMETHODCALLTYPE   endElement                  ( wchar_t*, int, wchar_t*, int, wchar_t*, int );
    HRESULT STDMETHODCALLTYPE   startDocument               ();
    HRESULT STDMETHODCALLTYPE   endDocument                 ()                                      { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   putDocumentLocator          ( ISAXLocator* )                        { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   startPrefixMapping          ( BSTR, int, BSTR, int )                { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   endPrefixMapping            ( BSTR, int )                           { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   characters                  ( BSTR, int )                           { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   ignorableWhitespace         ( BSTR, int )                           { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   processingInstruction       ( BSTR, int, BSTR, int )                { return NOERROR; }
    HRESULT STDMETHODCALLTYPE   skippedEntity               ( BSTR, int )                           { return NOERROR; }


    Content_handler*           _content_handler;
};

//DESCRIBE_CLASS( NULL, Content_handler_for_com, sax_content_handler_for_com, CLSID_Global, "Content_handler_for_com", "1.0", 0 );

//------------------------------------------------------------Content_handler_for_com::startElement

HRESULT STDMETHODCALLTYPE Content_handler_for_com::startElement( wchar_t* namespace_uri, int namespace_uri_len,
                                                                 wchar_t* local_name, int local_name_len,
                                                                 wchar_t* name, int name_len,
                                                                 ISAXAttributes *attributes )
{
    HRESULT hr = S_OK;

    try 
    {
        _content_handler->start_element( w_as_string( namespace_uri, namespace_uri_len ),
                                         w_as_string( local_name, local_name_len ),
                                         w_as_string( name, name_len ),
                                         Attributes(attributes) );
    }
    catch( const Xc& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//--------------------------------------------------------------Content_handler_for_com::endElement
       
HRESULT STDMETHODCALLTYPE Content_handler_for_com::endElement( wchar_t* namespace_uri, int namespace_uri_len,
                                                               wchar_t* local_name, int local_name_len,
                                                               wchar_t* name, int name_len )
{
    HRESULT hr = S_OK;

    try 
    {
        _content_handler->end_element( w_as_string( namespace_uri, namespace_uri_len ),
                                       w_as_string( local_name, local_name_len ),
                                       w_as_string( name, name_len ) );
    }
    catch( const Xc& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//------------------------------------------------------------Content_handler_for_com::startDocument

HRESULT STDMETHODCALLTYPE Content_handler_for_com::startDocument()
{
    HRESULT hr = S_OK;

    try 
    {
        _content_handler->start_document();
    }
    catch( const Xc& x )  { hr = _set_excepinfo(x); }

    return hr;
}
        
//-------------------------------------------------------------------------------Attributes::length

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

//-----------------------------------------------------------------------Reader::set_content_handler

void Reader::set_content_handler( Content_handler* content_handler )
{
    HRESULT hr;

    _content_handler_for_com->_content_handler = content_handler;

    hr = _delegated->putContentHandler( content_handler? _content_handler_for_com : NULL );

    if( FAILED(hr) )  throw_ole( hr, "SAXXMLReader::putContentHandler" );
}

//------------------------------------------------------------------------------------Reader::Reader

Reader::Reader()
:
    _delegated(NULL)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    hr = CoCreateInstance( __uuidof(SAXXMLReader), NULL, CLSCTX_ALL, __uuidof(ISAXXMLReader), (void**)&_delegated );
    if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance(SAXXMLReader)" );

    _content_handler_for_com = new Content_handler_for_com;
    _content_handler_for_com->AddRef();

    // SAXErrorHandlerImpl * pEc = new SAXErrorHandlerImpl();
    // hr = pRdr->putErrorHandler(pEc);
    // SAXDTDHandlerImpl * pDc = new SAXDTDHandlerImpl();
    // hr = pRdr->putDTDHandler(pDc);
}

//-----------------------------------------------------------------------------------Reader::~Reader

Reader::~Reader()
{
    if( _delegated )  _delegated->Release();
    if( _content_handler_for_com )  _content_handler_for_com->Release();

    CoUninitialize();
}

//-------------------------------------------------------------------------------------Reader::parse

void Reader::parse( const string& document )
{
    HRESULT  hr;
    Variant  document_variant = SysAllocString_string( document );

    hr = _delegated->parse( document_variant );
    if( FAILED(hr) )  throw_ole( hr, "SAXXMLReader::parse" );
}

//--------------------------------------------------------------------------------------------------

} //namespace sax
} //namespace xml
} //namespace sos

#else
    // Unix libxml
#endif