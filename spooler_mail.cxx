// $Id: spooler_mail.cxx,v 1.17 2003/03/20 19:21:38 jz Exp $


#include "spooler.h"
#include "spooler_mail.h"

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Com_mail, mail, CLSID_mail, "Spooler.Mail", "1.0" )

//-------------------------------------------------------------------------------Com_mail::_methods
#ifdef Z_COM

const Com_method Com_mail::_methods[] =
{ 
   // _flags              , _name                , _method                                     , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYPUT, 1, "to"              , (Com_method_ptr)&Com_mail::put_to           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 1, "to"              , (Com_method_ptr)&Com_mail::get_to           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 2, "from"            , (Com_method_ptr)&Com_mail::put_from         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 2, "from"            , (Com_method_ptr)&Com_mail::get_from         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 3, "cc"              , (Com_method_ptr)&Com_mail::put_cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 3, "cc"              , (Com_method_ptr)&Com_mail::get_cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 4, "bcc"             , (Com_method_ptr)&Com_mail::put_cc           , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 4, "bcc"             , (Com_method_ptr)&Com_mail::get_cc           , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 5, "subject"         , (Com_method_ptr)&Com_mail::put_subject      , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 5, "subject"         , (Com_method_ptr)&Com_mail::get_subject      , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 6, "body"            , (Com_method_ptr)&Com_mail::put_body         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 6, "body"            , (Com_method_ptr)&Com_mail::get_body         , VT_BSTR     },
    { DISPATCH_METHOD     , 7, "add_file"        , (Com_method_ptr)&Com_mail::add_file         , VT_EMPTY    , { VT_BSTR, VT_BSTR, VT_BSTR, VT_BSTR }, 3 },
    { DISPATCH_PROPERTYPUT, 8, "smtp"            , (Com_method_ptr)&Com_mail::put_smtp         , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 8, "smtp"            , (Com_method_ptr)&Com_mail::get_smtp         , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 9, "queue_dir"       , (Com_method_ptr)&Com_mail::put_queue_dir    , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 9, "queue_dir"       , (Com_method_ptr)&Com_mail::get_queue_dir    , VT_BSTR     },
    { DISPATCH_METHOD     ,10, "add_header_field", (Com_method_ptr)&Com_mail::add_header_field , VT_EMPTY    , { VT_BSTR, VT_BSTR } },
    { DISPATCH_METHOD     ,11, "dequeue"         , (Com_method_ptr)&Com_mail::dequeue          , VT_INT      },
    { DISPATCH_PROPERTYGET,12, "dequeue_log"     , (Com_method_ptr)&Com_mail::get_dequeue_log  , VT_BSTR     },
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
    if( iid == IID_Ihas_java_class_name )  
    { 
        AddRef();
        *result = (Ihas_java_class_name*)this;  
        return S_OK; 
    }

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

//---------------------------------------------------------------------------------Com_mail::put_to

STDMETHODIMP Com_mail::put_to( BSTR to )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_to( bstr_as_string(to) );
        _to = _msg->to();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_mail::put_cc

STDMETHODIMP Com_mail::put_cc( BSTR cc )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_cc( bstr_as_string(cc) );
        _cc = _msg->cc().c_str();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_mail::put_bcc

STDMETHODIMP Com_mail::put_bcc( BSTR bcc )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_bcc( bstr_as_string(bcc) );
        _bcc = _msg->bcc().c_str();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::put_from

STDMETHODIMP Com_mail::put_from( BSTR from )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_from( bstr_as_string(from) );
        _from = _msg->from().c_str();
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_from

STDMETHODIMP Com_mail::get_from( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *result = SysAllocString_string( _msg->from() );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_mail::put_subject

STDMETHODIMP Com_mail::put_subject( BSTR subject )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _subject = subject;
        _msg->set_subject( bstr_as_string( subject ) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_mail::get_subject

STDMETHODIMP Com_mail::get_subject( BSTR* result )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        *result = SysAllocString_string( _msg->subject() );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::put_body

STDMETHODIMP Com_mail::put_body( BSTR body )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_body( bstr_as_string( body ) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_body

STDMETHODIMP Com_mail::get_body( BSTR* result )
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

STDMETHODIMP Com_mail::add_file( BSTR real_filename, BSTR mail_filename, BSTR content_type, BSTR encoding )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->add_file( bstr_as_string(real_filename), bstr_as_string(mail_filename), bstr_as_string(content_type), bstr_as_string(encoding) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_mail::add_attachment
/*
STDMETHODIMP Com_mail::add_attachment( BSTR filename, BSTR content_type )
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

STDMETHODIMP Com_mail::put_smtp( BSTR smtp )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->set_smtp( bstr_as_string(smtp) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const exception & x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_smtp

STDMETHODIMP Com_mail::get_smtp( BSTR* smtp )
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

STDMETHODIMP Com_mail::put_queue_dir( BSTR queue_dir )
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

STDMETHODIMP Com_mail::get_queue_dir( BSTR* queue_dir )
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

STDMETHODIMP Com_mail::add_header_field( BSTR field_name, BSTR value )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try 
    {
        _msg->add_header_field( bstr_as_string(field_name), bstr_as_string(value) );
    }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }
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

//--------------------------------------------------------------------------------Com_mail::dequeue

STDMETHODIMP Com_mail::dequeue( int* result )
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
