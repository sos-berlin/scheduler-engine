// $Id: spooler_mail.cxx,v 1.2 2002/03/21 19:04:31 jz Exp $


#include "../kram/sos.h"
#include "spooler.h"
#include "spooler_mail.h"

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

static _bstr_t empty_bstr;

//------------------------------------------------------------------------------------Typbibliothek

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Com_mail, mail, spooler_com::CLSID_mail, "Spooler.Mail", "1.0" )

//-------------------------------------------------------------------------------Com_mail::Com_mail

Com_mail::Com_mail( Spooler* spooler )
: 
    Sos_ole_object( mail_class_ptr, this ),
    _zero_(this+1),
    _spooler(spooler)
{
}

//------------------------------------------------------------------------------Com_mail::~Com_mail

Com_mail::~Com_mail()
{
    _msg = NULL;
}

//-----------------------------------------------------------------------------------Com_mail::init

void Com_mail::init()
{
    if( _msg == NULL )
    {
        _msg = SOS_NEW( Jmail_message );
        _msg->init();
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
        _to.Empty();
        _to.Attach( SysAllocString_string( _msg->to() ) );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.subject" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.body" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::add_file

STDMETHODIMP Com_mail::add_file( BSTR filename, BSTR content_type )
{
    HRESULT hr = NOERROR;

    if( _msg == 0 )  return E_POINTER;

    try
    {
        _msg->add_file( bstr_as_string( filename ), bstr_as_string( content_type ) );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

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
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.smtp" ); }

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
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_header_field" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_mail::send

void Com_mail::send()
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Com_mail::send" );

    _spooler->_log.debug( "email " + _msg->to() + ": " + _msg->subject() );

    _msg->send();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

