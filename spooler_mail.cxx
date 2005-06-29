// $Id$


#include "spooler.h"
#include "spooler_mail.h"

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Com_mail, mail, CLSID_Mail, "Spooler.Mail", "1.0" )

//-------------------------------------------------------------------------------Com_mail::_methods
#ifdef Z_COM

const Com_method Com_mail::_methods[] =
{ 
   // _flags              , _name                , _method                                     , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT, 1, "to"              , (Com_method_ptr)&Com_mail::put_To           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 1, "to"              , (Com_method_ptr)&Com_mail::get_To           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 2, "from"            , (Com_method_ptr)&Com_mail::put_From         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 2, "from"            , (Com_method_ptr)&Com_mail::get_From         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 3, "cc"              , (Com_method_ptr)&Com_mail::put_Cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 3, "cc"              , (Com_method_ptr)&Com_mail::get_Cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 4, "bcc"             , (Com_method_ptr)&Com_mail::put_Cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 4, "bcc"             , (Com_method_ptr)&Com_mail::get_Cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 5, "subject"         , (Com_method_ptr)&Com_mail::put_Subject      , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 5, "subject"         , (Com_method_ptr)&Com_mail::get_Subject      , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 6, "body"            , (Com_method_ptr)&Com_mail::put_Body         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 6, "body"            , (Com_method_ptr)&Com_mail::get_Body         , VT_BSTR     },
    { DISPATCH_METHOD     , 7, "add_file"        , (Com_method_ptr)&Com_mail::Add_file         , VT_EMPTY    , { VT_BSTR, VT_BSTR, VT_BSTR, VT_BSTR }, 3 },
    { DISPATCH_PROPERTYPUT, 8, "smtp"            , (Com_method_ptr)&Com_mail::put_Smtp         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 8, "smtp"            , (Com_method_ptr)&Com_mail::get_Smtp         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 9, "queue_dir"       , (Com_method_ptr)&Com_mail::put_Queue_dir    , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 9, "queue_dir"       , (Com_method_ptr)&Com_mail::get_Queue_dir    , VT_BSTR     },
    { DISPATCH_METHOD     ,10, "add_header_field", (Com_method_ptr)&Com_mail::Add_header_field , VT_EMPTY    , { VT_BSTR, VT_BSTR } },
    { DISPATCH_METHOD     ,11, "dequeue"         , (Com_method_ptr)&Com_mail::Dequeue          , VT_INT      },
    { DISPATCH_PROPERTYGET,12, "dequeue_log"     , (Com_method_ptr)&Com_mail::get_Dequeue_log  , VT_BSTR     },
    { DISPATCH_PROPERTYGET,13, "Java_class_name" , (Com_method_ptr)&Com_mail::get_Java_class_name, VT_BSTR },
    {}
};

#endif
//-------------------------------------------------------------------------------Com_mail::Com_mail

Com_mail::Com_mail( Spooler* spooler )
: 
    Sos_ole_object( mail_class_ptr, (Imail*)this ),
    _zero_(this+1),
    _spooler(spooler)
{
}

//------------------------------------------------------------------------------Com_mail::~Com_mail

Com_mail::~Com_mail()
{
    _msg = NULL;
}

//-------------------------------------------------------------------------Com_mail::QueryInterface

STDMETHODIMP Com_mail::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}

//-----------------------------------------------------------------------------------Com_mail::init

void Com_mail::init()
{
    if( _msg == NULL )
    {
        _msg = mail::create_message( _spooler->_java_vm );
    }
}

//----------------------------------------------------------------------------Com_mail::dom_element

xml::Element_ptr Com_mail::dom_element( const xml::Document_ptr& dom )
{
    xml::Element_ptr mail_element = dom.createElement( "mail" );
  //mail_element.setAttribute_optional( "smtp"   , _smtp    );

    xml::Element_ptr header_element = mail_element.append_new_element( "header" );

    
    header_element.setAttribute_optional( "from"   , _from    );
    header_element.setAttribute_optional( "to"     , _to      );
    header_element.setAttribute_optional( "cc"     , _cc      );
    header_element.setAttribute_optional( "bcc"    , _bcc     );
  //header_element.setAttribute_optional( "subject", _subject );
    header_element.append_new_text_element( "subject", _subject );

    Z_FOR_EACH( Header_fields, _header_fields, h )
    {
        xml::Element_ptr field_element = header_element.append_new_element( "field" );
        field_element.setAttribute         ( "name" , h->first  );
        field_element.setAttribute_optional( "value", h->second );
    }


    xml::Element_ptr body_element = mail_element.append_new_element( "body" );
    body_element.append_new_text_element( "text"   , _body    );


    Z_FOR_EACH( list<File>, _files, f )
    {
        xml::Element_ptr file_element = body_element.append_new_element( "file" );

        file_element.setAttribute_optional( "path"         , f->_real_filename );
        file_element.setAttribute_optional( "mail_filename", f->_mail_filename );
        file_element.setAttribute_optional( "content_type" , f->_content_type  );
        file_element.setAttribute_optional( "encoding"     , f->_encoding      );
    }

    return mail_element;
}

//--------------------------------------------------------------------------------Com_mail::set_dom

void Com_mail::set_dom( const xml::Element_ptr& mail_element )
{
    if( !mail_element )  throw_xc( "Com_mail::set_dom", "NULL pointer" );

    init();

    xml::Element_ptr header_element = mail_element.select_node_strict( "header" );

    set_from   ( header_element.getAttribute( "from"    ) );
    set_to     ( header_element.getAttribute( "to"      ) );
    set_cc     ( header_element.getAttribute( "cc"      ) );
    set_bcc    ( header_element.getAttribute( "bcc"     ) );
  //set_smtp   ( header_element.getAttribute( "smtp"    ) );
  //set_subject( header_element.getAttribute( "subject" ) );
    set_subject( xml::Element_ptr( header_element.select_node( "subject" ) ).trimmed_text() );

    xml::Xpath_nodes field_elements = mail_element.select_nodes( "header/field" );
    for( int i = 0; i < field_elements.count(); i++ )
    {
        xml::Element_ptr field_element = field_elements[ i ];

        add_header_field( field_element.getAttribute( "name" ),
                          field_element.getAttribute( "value" ) );
    }


    set_body( xml::Element_ptr( mail_element.select_node( "body/text" ) ).trimmed_text() );

    xml::Xpath_nodes file_elements = mail_element.select_nodes( "body/file" );
    for( int i = 0; i < file_elements.count(); i++ )
    {
        xml::Element_ptr file_element = file_elements[ i ];

        add_file( file_element.getAttribute( "path" ),
                  file_element.getAttribute( "mail_filename" ),
                  file_element.getAttribute( "content_type"  ),
                  file_element.getAttribute( "encoding"      ) );
    }
}

//----------------------------------------------------------------------------Com_mail::set_subject

void Com_mail::set_subject( const string& subject )
{
    _msg->set_subject( subject );
    _subject = subject;
}

//-------------------------------------------------------------------------------Com_mail::set_from

void Com_mail::set_from( const string& from )
{
    _msg->set_from( from );
    _from = _msg->from();
}

//--------------------------------------------------------------------------Com_mail::set_from_name

void Com_mail::set_from_name( const string& from_name )
{
    _msg->set_from_name( from_name );
    _from = _msg->from();
}

//---------------------------------------------------------------------------------Com_mail::set_to

void Com_mail::set_to( const string& to )
{
    _msg->set_to( to );
    _to = _msg->to();
}

//---------------------------------------------------------------------------------Com_mail::set_cc

void Com_mail::set_cc( const string& cc )
{
    _msg->set_cc( cc );
    _cc = _msg->cc();
}

//--------------------------------------------------------------------------------Com_mail::set_bcc

void Com_mail::set_bcc( const string& bcc )
{
    _msg->set_bcc( bcc );
    _bcc = _msg->bcc();
}

//-------------------------------------------------------------------------------Com_mail::set_body

void Com_mail::set_body( const string& body )
{
    _msg->set_body( body );
    _body = body;
}

//-------------------------------------------------------------------------------Com_mail::set_smtp

void Com_mail::set_smtp( const string& smtp )
{
    _smtp = smtp;
    _msg->set_smtp( smtp );
}

//-----------------------------------------------------------------------Com_mail::add_header_field

void Com_mail::add_header_field( const string& name, const string& value )
{
    _msg->add_header_field( name, value );
    _header_fields.push_back( pair<string,string>( name, value ) );
}

//-------------------------------------------------------------------------------Com_mail::add_file

void Com_mail::add_file( const string& real_filename, const string& mail_filename, const string& content_type, const string& encoding )
{
    _msg->add_file( real_filename, mail_filename, content_type, encoding );

    File file;
    file._real_filename = real_filename;
    file._mail_filename = mail_filename;
    file._content_type  = content_type;
    file._encoding      = encoding;
    _files.push_back( file );
}

//---------------------------------------------------------------------------------Com_mail::put_To

STDMETHODIMP Com_mail::put_To( BSTR to )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_to( string_from_bstr(to) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_mail::put_Cc

STDMETHODIMP Com_mail::put_Cc( BSTR cc )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_cc( string_from_bstr( cc ) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_mail::put_Bcc

STDMETHODIMP Com_mail::put_Bcc( BSTR bcc )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_bcc( bstr_as_string(bcc) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::put_From

STDMETHODIMP Com_mail::put_From( BSTR from )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_from( bstr_as_string(from) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_from

STDMETHODIMP Com_mail::get_From( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        hr = String_to_bstr( _msg->from(), result );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_mail::put_subject

STDMETHODIMP Com_mail::put_Subject( BSTR subject )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_subject( bstr_as_string( subject ) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_mail::get_subject

STDMETHODIMP Com_mail::get_Subject( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        hr = String_to_bstr( _msg->subject(), result );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::put_body

STDMETHODIMP Com_mail::put_Body( BSTR body )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_body( bstr_as_string( body ) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_body

STDMETHODIMP Com_mail::get_Body( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *result = SysAllocString_string( _msg->body() );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::add_file

STDMETHODIMP Com_mail::Add_file( BSTR real_filename, BSTR mail_filename, BSTR content_type, BSTR encoding )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        add_file( bstr_as_string(real_filename), bstr_as_string(mail_filename), bstr_as_string(content_type), bstr_as_string(encoding) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_mail::add_attachment
/*
STDMETHODIMP Com_mail::Add_attachment( BSTR filename, BSTR content_type )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->AddAttachment( filename, content_type );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

    return hr;
}
*/
//-------------------------------------------------------------------------------Com_mail::put_smtp

STDMETHODIMP Com_mail::put_Smtp( BSTR smtp )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        set_smtp( bstr_as_string(smtp) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_smtp

STDMETHODIMP Com_mail::get_Smtp( BSTR* smtp )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *smtp = SysAllocString_string( _msg->smtp() );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_mail::put_queue_dir

STDMETHODIMP Com_mail::put_Queue_dir( BSTR queue_dir )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_queue_dir( bstr_as_string(queue_dir) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_mail::get_queue_dir

STDMETHODIMP Com_mail::get_Queue_dir( BSTR* queue_dir )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *queue_dir = SysAllocString_string( _msg->queue_dir() );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_mail::put_header_field

STDMETHODIMP Com_mail::Add_header_field( BSTR field_name, BSTR value )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try 
    {
        add_header_field( bstr_as_string(field_name), bstr_as_string(value) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }

    return hr;
}

//--------------------------------------------------------------------Com_mail::get_Xslt_stylesheet

STDMETHODIMP Com_mail::get_Xslt_stylesheet( Ixslt_stylesheet** result )
{
    HRESULT hr = NOERROR;

    try 
    {
        *result = NULL;
        ptr<Xslt_stylesheet> stylesheet = xslt_stylesheet();
        if( stylesheet )  *result = stylesheet.copy();
    }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_mail::send

int Com_mail::send()
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Com_mail::send" );

    _spooler->_log.debug( "email " + _msg->to() + ": " + _msg->subject() );

    return _msg->send();
}

//------------------------------------------------------------------------Com_mail::xslt_stylesheet

ptr<Xslt_stylesheet> Com_mail::xslt_stylesheet()
{
    if( !_xslt_stylesheet  &&  _xslt_stylesheet_path != "" )
    {
        ptr<Xslt_stylesheet> stylesheet = Z_NEW( Xslt_stylesheet );
        stylesheet->load_file( _xslt_stylesheet_path );
        
        _xslt_stylesheet = stylesheet;
    }

    return _xslt_stylesheet;
}

//--------------------------------------------------------------------------------Com_mail::dequeue

STDMETHODIMP Com_mail::Dequeue( int* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *result = _msg->dequeue();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.dequeue" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.dequeue" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
