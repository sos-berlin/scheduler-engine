// $Id$


#include "spooler.h"
#include "spooler_mail.h"
#include "../zschimmer/z_mail.h"

namespace sos {
namespace scheduler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

//extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Com_mail, mail, CLSID_Mail, "Spooler.Mail", "1.0" )

//---------------------------------------------------------------------Mail_defaults::Mail_defaults

Mail_defaults::Mail_defaults( Spooler* spooler )
{
    if( spooler  &&  this != &spooler->_mail_defaults )
    {
        _map = spooler->_mail_defaults._map;
    }
}

//-------------------------------------------------------------------------Mail_defaults::has_value

bool Mail_defaults::has_value( const string& name ) const
{ 
    Map::const_iterator it = _map.find( name );
    return it != _map.end() &&  it->second != "";
}

//------------------------------------------------------------------------Mail_defaults::operator[]

string Mail_defaults::operator[] ( const string& name ) const
{ 
    Map::const_iterator it = _map.find( name );  
    return it == _map.end()? "" : it->second; 
}

//-----------------------------------------------------------------------------Mail_defaults::value

string Mail_defaults::value( const string& name ) const
{ 
    string result = (*this)[ name ];
    return result == "-"? "" : result;
}

//-------------------------------------------------------------------------------Com_mail::_methods
#ifdef Z_COM

const Com_method Com_mail::_methods[] =
{
   // _flags              , _name                , _method                                     , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT,  1, "to"              , (Com_method_ptr)&Com_mail::put_To           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  1, "to"              , (Com_method_ptr)&Com_mail::get_To           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  2, "from"            , (Com_method_ptr)&Com_mail::put_From         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  2, "from"            , (Com_method_ptr)&Com_mail::get_From         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  3, "cc"              , (Com_method_ptr)&Com_mail::put_Cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  3, "cc"              , (Com_method_ptr)&Com_mail::get_Cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  4, "bcc"             , (Com_method_ptr)&Com_mail::put_Cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  4, "bcc"             , (Com_method_ptr)&Com_mail::get_Cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  5, "subject"         , (Com_method_ptr)&Com_mail::put_Subject      , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  5, "subject"         , (Com_method_ptr)&Com_mail::get_Subject      , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  6, "body"            , (Com_method_ptr)&Com_mail::put_Body         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  6, "body"            , (Com_method_ptr)&Com_mail::get_Body         , VT_BSTR     },
    { DISPATCH_METHOD     ,  7, "add_file"        , (Com_method_ptr)&Com_mail::Add_file         , VT_EMPTY    , { VT_BSTR, VT_BSTR, VT_BSTR, VT_BSTR }, 3 },
    { DISPATCH_PROPERTYPUT,  8, "smtp"            , (Com_method_ptr)&Com_mail::put_Smtp         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  8, "smtp"            , (Com_method_ptr)&Com_mail::get_Smtp         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT,  9, "queue_dir"       , (Com_method_ptr)&Com_mail::put_Queue_dir    , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  9, "queue_dir"       , (Com_method_ptr)&Com_mail::get_Queue_dir    , VT_BSTR     },
    { DISPATCH_METHOD     , 10, "add_header_field", (Com_method_ptr)&Com_mail::Add_header_field , VT_EMPTY    , { VT_BSTR, VT_BSTR } },
    { DISPATCH_METHOD     , 11, "dequeue"         , (Com_method_ptr)&Com_mail::Dequeue          , VT_INT      },
    { DISPATCH_PROPERTYGET, 12, "dequeue_log"     , (Com_method_ptr)&Com_mail::get_Dequeue_log  , VT_BSTR     },
    { DISPATCH_PROPERTYGET, 13, "Xslt_stylesheet_path" , (Com_method_ptr)&Com_mail::get_Xslt_stylesheet_path, VT_BSTR },
    { DISPATCH_PROPERTYPUT, 13, "Xslt_stylesheet_path" , (Com_method_ptr)&Com_mail::put_Xslt_stylesheet_path, VT_EMPTY, { VT_BSTR } },
    { DISPATCH_PROPERTYPUT, 14, "Xslt_stylesheet" , (Com_method_ptr)&Com_mail::get_Xslt_stylesheet, VT_DISPATCH },
    { DISPATCH_PROPERTYGET, 15, "Java_class_name" , (Com_method_ptr)&Com_mail::get_Java_class_name, VT_BSTR },
    {}
};

#endif
//-------------------------------------------------------------------------------Com_mail::Com_mail

Com_mail::Com_mail( Spooler* spooler )
:
    Sos_ole_object( mail_class_ptr, (Imail*)this ),
    _zero_(this+1),
    _spooler(spooler),
    _defaults(spooler)
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
        _msg = mail::create_message( _spooler->java_subsystem()->java_vm() );
    }
}

//----------------------------------------------------------------------------Com_mail::dom_element

xml::Element_ptr Com_mail::dom_element( const xml::Document_ptr& dom )
{
    xml::Element_ptr mail_element = dom.createElement( "mail" );
  //mail_element.setAttribute_optional( "smtp"   , _smtp    );

    xml::Element_ptr header_element = mail_element.append_new_element( "header" );


    Email_address from;
    from.suppress_exceptions( true );
    from = _from;
    header_element.setAttribute_optional( "from_address", from.address() );
    header_element.setAttribute_optional( "from_name"   , from.name()    );

    header_element.setAttribute_optional( "to"     , _to      );
    header_element.setAttribute_optional( "cc"     , _cc      );
    header_element.setAttribute_optional( "bcc"    , _bcc     );
  //header_element.setAttribute_optional( "subject", _subject );

    if( _subject != "" )
    header_element.append_new_text_element( "subject", _subject );

    Z_FOR_EACH( Header_fields, _header_fields, h )
    {
        xml::Element_ptr field_element = header_element.append_new_element( "field" );
        field_element.setAttribute         ( "name" , h->first  );
        field_element.setAttribute_optional( "value", h->second );
    }


    xml::Element_ptr body_element = mail_element.append_new_element( "body" );

    if( _body != "" )
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
    if( !mail_element )  return;

    if( !mail_element )  throw_xc( "Com_mail::set_dom", "NULL pointer" );

    init();


    if( xml::Element_ptr header_element = mail_element.select_node( "header" ) )
    {
        // getAttribute() liefert "", wenn das Attribut nicht angegeben ist.

        string value;
        value = header_element.getAttribute( "from_address" );    if( value != "" )  set_from     ( value );
        value = header_element.getAttribute( "from_name"    );    if( value != "" )  set_from_name( value == "-"? "" : value );
        value = header_element.getAttribute( "to"           );    if( value != "" )  set_to       ( value );
        value = header_element.getAttribute( "cc"           );    if( value != "" )  set_cc       ( value == "-"? "" : value );
        value = header_element.getAttribute( "bcc"          );    if( value != "" )  set_bcc      ( value == "-"? "" : value );
      //set_smtp     ( header_element.getAttribute( "smtp"      ) );

        if( xml::Element_ptr e = header_element.select_node( "subject" ) )  set_subject( e.trimmed_text() );


        xml::Xpath_nodes field_elements = header_element.select_nodes( "field" );
        for( int i = 0; i < field_elements.count(); i++ )
        {
            xml::Element_ptr field_element = field_elements[ i ];

            add_header_field( field_element.getAttribute( "name" ),
                              field_element.getAttribute( "value" ) );
        }
    }


    if( xml::Element_ptr e = mail_element.select_node( "body/text" ) )  set_body( e.trimmed_text() );

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

//---------------------------------------------------------------------Com_mail::value_with_default

string Com_mail::value_with_default( const string& value, const string& default_name )
{
    if( value != "" )  return value;
    
    return _defaults.value( default_name );
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

//-------------------------------------------------------------------------------------Com_mail::to

string Com_mail::to() 
{
    return value_with_default( _to, "to" );
}

//---------------------------------------------------------------------------------Com_mail::set_cc

void Com_mail::set_cc( const string& cc )
{
    _msg->set_cc( cc );
    _cc = _msg->cc();
    _cc_set = true;
}

//-------------------------------------------------------------------------------------Com_mail::cc

string Com_mail::cc() 
{
    return _cc_set? _cc : _defaults.value( "cc" );
}

//--------------------------------------------------------------------------------Com_mail::set_bcc

void Com_mail::set_bcc( const string& bcc )
{
    _msg->set_bcc( bcc );
    _bcc = _msg->bcc();
    _bcc_set = true;
}

//------------------------------------------------------------------------------------Com_mail::bcc

string Com_mail::bcc() 
{
    return _bcc_set? _bcc : _defaults.value( "bcc" );
}

//-------------------------------------------------------------------------------Com_mail::set_body

void Com_mail::set_body( const string& body )
{
    _msg->set_body( body );
    _body = body;
}

//-----------------------------------------------------------------------------------Com_mail::body

string Com_mail::body() 
{
    return value_with_default( _body, "body" );
}

//-------------------------------------------------------------------------------Com_mail::set_smtp

void Com_mail::set_smtp( const string& smtp )
{
    _smtp = smtp;
    _msg->set_smtp( smtp );
}

//-----------------------------------------------------------------------------------Com_mail::smtp

string Com_mail::smtp() 
{
    string result = value_with_default( _smtp, "smtp" );
    if( result == "" )  result = _msg->smtp();
    return result;
}

//--------------------------------------------------------------------------Com_mail::set_queue_dir

void Com_mail::set_queue_dir( const string& queue_dir )
{
    _msg->set_queue_dir( queue_dir);
}

//------------------------------------------------------------------------------Com_mail::queue_dir

string Com_mail::queue_dir()
{
    return value_with_default( _msg->queue_dir(), "queue_dir" );
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

//-------------------------------------------------------------------------Com_mail::add_attachment

void Com_mail::add_attachment( const string& data, const string& mail_filename, const string& content_type, const string& encoding )
{
    _msg->add_attachment( data, mail_filename, content_type, encoding );

    /* Nur zum Debuggen benutzt, brauchen wir nicht in XML

    File file;
    file._real_filename = real_filename;
    file._mail_filename = mail_filename;
    file._content_type  = content_type;
    file._encoding      = encoding;
    _files.push_back( file );
    */
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
        hr = String_to_bstr( value_with_default( _msg->body(), "from" ), result );
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
        hr = String_to_bstr( value_with_default( _msg->subject(), "subject" ), result );
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
        *result = SysAllocString_string( value_with_default( _msg->body(), "body" ) );
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
        *smtp = SysAllocString_string( this->smtp() );
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
        set_queue_dir( bstr_as_string(queue_dir) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.queue_dir" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_mail::get_queue_dir

STDMETHODIMP Com_mail::get_Queue_dir( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *result = SysAllocString_string( queue_dir() );
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

int Com_mail::send( const Mail_defaults& defaults )
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Com_mail::send" );

    use_defaults( defaults );


    if( _from == ""  ||  _to == ""  ||  _subject == ""  ||  _body == "" )
    {
        _spooler->log()->warn( message_string( "SCHEDULER-292" ) );   //"Email unterdrückt, weil From, To, Subject oder der Nachrichtentext fehlt"
        return 0;
    }

    _spooler->log()->debug( "email " + _msg->to() + ": " + _msg->subject() );


    const double throttle = 1.0;
    if( _spooler->_last_mail_timestamp + throttle > Time::now() )    // eMail-Drossel
    {
        Z_LOG2( "scheduler", "eMail-Drossel " << throttle << "s\n" );
        sleep( throttle );
    }


    bool result = _msg->send();

    _spooler->_last_mail_timestamp = Time::now();

    _spooler->assert_is_still_active( __FUNCTION__ );

    return result;
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
        use_queue_defaults( _spooler->_mail_defaults );
        use_smtp_default( _spooler->_mail_defaults );

        *result = _msg->dequeue();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.dequeue" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.dequeue" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_mail::use_defaults

void Com_mail::use_defaults( const Mail_defaults& defaults )
{
    Email_address from        ( _from );
    Email_address default_from( defaults[ "from" ] );

    if( from.address() == "" )  from.set_address( default_from.address() );
    if( from.name   () == "" )  from.set_name   ( default_from.name() != ""? default_from.name() : defaults[ "from_name" ] );
    set_from( from );
  //if( from.address()  == ""  &&  defaults.has_value( "from"      ) )  set_from     ( defaults[ "from"      ] );
  //if( from.name()     == ""  &&  defaults.has_value( "from_name" ) )  set_from_name( defaults[ "from_name" ] );

    if( _to             == ""  &&  defaults.has_value( "to"        ) )  set_to       ( defaults[ "to"        ] );
    if( !_cc_set               &&  defaults.has_value( "cc"        ) )  set_cc       ( defaults[ "cc"        ] );
    if( !_bcc_set              &&  defaults.has_value( "bcc"       ) )  set_bcc      ( defaults[ "bcc"       ] );
    if( _subject        == ""  &&  defaults.has_value( "subject"   ) )  set_subject  ( defaults[ "subject"   ] );
    if( _body           == ""  &&  defaults.has_value( "body"      ) )  set_body     ( defaults[ "body"      ] );


    use_queue_defaults( defaults );     // Nach dequeue() ist der Default von _spooler->_mail_defaults genommen.
    use_smtp_default( defaults );       // Nach dequeue() ist der Default von _spooler->_mail_defaults genommen.
}

//---------------------------------------------------------------------Com_mail::use_queue_defaults

void Com_mail::use_queue_defaults( const Mail_defaults& defaults )
{
    if( defaults.has_value( "queue_dir" )  &&  defaults[ "queue_dir" ] != "-" )  set_queue_dir( defaults[ "queue_dir" ] );
}

//-----------------------------------------------------------------------Com_mail::use_smtp_default

void Com_mail::use_smtp_default( const Mail_defaults& defaults )
{
    if( defaults[ "queue_only" ] == "1" )  _msg->set_queue_only( true );
    if( defaults[ "queue_only" ] == "0" )  _msg->set_queue_only( false );

    if( defaults.has_value( "smtp"      )  &&  defaults[ "smtp"      ] != "-" )  set_smtp     ( defaults[ "smtp"      ] );
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
