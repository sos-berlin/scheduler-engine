// $Id: spooler_mail_jmail.cxx,v 1.1 2002/03/03 17:00:09 jz Exp $


#include "../kram/sos.h"
#include "spooler.h"
#include "spooler_mail_jmail.h"

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

static _bstr_t empty_bstr;


//---------------------------------------------------------------------------------------Email_addr

struct Email_addr
{
                                Email_addr                  ()                                      {}
                                Email_addr                  ( const wchar_t* );

    const wchar_t*              parse                       ( const wchar_t* );

    string                     _addr;
    string                     _name;
};

//------------------------------------------------------------------------------------Typbibliothek

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Com_mail, mail, spooler_com::CLSID_mail, "Spooler.Mail", "1.0" )

//----------------------------------------------------------------------------------------make_addr

static string make_addr( const _bstr_t& addr, const _bstr_t& name )
{
    if( SysStringLen(name) == 0  ||  addr == name ) 
    {
        return bstr_as_string(addr);
    }
    else
    {
        return bstr_as_string(name) + " <" + bstr_as_string(addr) + ">";
    }
}

//----------------------------------------------------------------------------------------make_addr

static void make_addr( BSTR* result_bstr, const _bstr_t& addr, const _bstr_t& name )
{
    string result = make_addr( addr, name );
    *result_bstr = SysAllocString_string( result );
}

//---------------------------------------------------------------------------Email_addr::Email_addr

Email_addr::Email_addr( const wchar_t* addr )
{
    if( !addr )  return;

    const wchar_t* t = addr;
    t = parse( t );
    if( *t )  throw_xc( "SPOOLER-135", w_as_string(addr) );
}

//--------------------------------------------------------------------------------Email_addr::parse

const wchar_t* Email_addr::parse( const wchar_t* t )
{
    _name = "";
    _addr = "";

    if( !t )  return L"";

    while( *t  &&   *t != '<' )  _name += *t++;

    if( *t == '<' )
    {
        t++;
        while( *t  &&  *t != '>' )  _addr += *t++;
        if( *t == '>' )  t++;
    }
    else
        _addr = _name;   // eMail-Adresse in _name, weil jmail sonst leeren Namen "" <x@x.xx> erzeugt

    trim( &_name );
    trim( &_addr );

    while( *t == ' ' )  t++;

    return t;
}

//-------------------------------------------------------------------------------Com_mail::Com_mail

Com_mail::Com_mail()
: 
    Sos_ole_object( mail_class_ptr, this ),
    _zero_(this+1)
{
}

//-----------------------------------------------------------------------------------Com_mail::init

void Com_mail::init()
{
    HRESULT hr;

    if( _msg == NULL )
    {
        hr = _msg.CreateInstance( __uuidof( jmail::Message ), NULL );
        if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", "jmail::Message" );

        //_msg->SimpleLayout = true;
    }
}

//----------------------------------------------------------------------Com_mail::add_to_recipients

void Com_mail::add_to_recipients( BSTR recipients, char recipient_type )
//void Com_mail::add_to_recipients( jmail::IRecipientsPtr result, BSTR recipients, char recipient_type )
{
    const wchar_t* t = recipients;
    if( !t )  return;

    while( *t )
    {
        Email_addr addr;
        t = addr.parse(t);

        _bstr_t name_bstr = addr._name.c_str();
        _bstr_t addr_bstr = addr._addr.c_str();

        switch( recipient_type )
        {
            case 0: _msg->AddRecipient   ( addr_bstr, name_bstr, empty_bstr );  break;
            case 1: _msg->AddRecipientCC ( addr_bstr, name_bstr, empty_bstr );  break;
            case 2: _msg->AddRecipientBCC( addr_bstr,            empty_bstr );  break;
        };

        //jmail::IRecipientPtr r = __uuidof( jmail::IRecipient );
        //r->New( name.c_str(), addr.c_str(), recipient_type );
        //if( FAILED(hr) )  throw_ole( hr, "jmail::Imessage::AddRecipient" );

        //result->Add( r );

        if( !*t )  break;

        if( *t != ',' )  throw_xc( "SPOOLER-135", bstr_as_string(recipients) );
        t++;
    }
}

//-------------------------------------------------------------------------Com_mail::get_recipients

void Com_mail::get_recipients( CComBSTR& result_bstr )
{
    string result;

    jmail::IRecipientsPtr recipients = _msg->Recipients;
    int n = recipients->Count;
    
    for( int i = 0; i < n; i++ )
    {
        if( i > 0 )  result += ", ";
        jmail::IRecipientPtr r = recipients->Item[i];
        result += make_addr( r->EMail, r->Name );
    }

    result_bstr = result.c_str();
}

//---------------------------------------------------------------------------------Com_mail::put_to

STDMETHODIMP Com_mail::put_to( BSTR to )
{
    HRESULT hr = NOERROR;

    if( SysStringLen(to) == 0 )  return hr;

    try
    {
        _msg->ClearRecipients();
        add_to_recipients( to , 0 );
        get_recipients( _to );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.to" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_mail::put_cc

STDMETHODIMP Com_mail::put_cc( BSTR cc )
{
    HRESULT hr = NOERROR;

    if( SysStringLen(cc) == 0 )  return hr;

    try
    {
        add_to_recipients( cc , 1 );       // Zum Prüfen
        get_recipients( _cc );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.cc" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_mail::put_bcc

STDMETHODIMP Com_mail::put_bcc( BSTR bcc )
{
    HRESULT hr = NOERROR;

    if( SysStringLen(bcc) == 0 )  return hr;

    try
    {
        add_to_recipients( bcc , 2 );       // Zum Prüfen
        get_recipients( _bcc );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.bcc" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::put_from

STDMETHODIMP Com_mail::put_from( BSTR from )
{
    HRESULT hr = NOERROR;

    if( SysStringLen(from) == 0 )  return hr;

    try
    {
        Email_addr addr = from;
        
        _msg->From     = addr._addr.c_str();
        _msg->FromName = addr._name.c_str();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_mail::get_from

STDMETHODIMP Com_mail::get_from( BSTR* result )
{
    HRESULT hr = NOERROR;

    try
    {
        make_addr( result, _msg->From, _msg->FromName );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.from" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_mail::put_subject

STDMETHODIMP Com_mail::put_subject( BSTR subject )
{
    if( _msg == 0 )  return E_POINTER;
    return _msg->put_Subject( subject );
}

//----------------------------------------------------------------------------Com_mail::get_subject

STDMETHODIMP Com_mail::get_subject( BSTR* subject )
{
    if( _msg == 0 )  return E_POINTER;
    return _msg->get_Subject( subject );
}

//-------------------------------------------------------------------------------Com_mail::put_body

STDMETHODIMP Com_mail::put_body( BSTR body )
{
    if( _msg == 0 )  return E_POINTER;
    return _msg->put_Body( body );
}

//-------------------------------------------------------------------------------Com_mail::get_body

STDMETHODIMP Com_mail::get_body( BSTR* body )
{
    if( _msg == 0 )  return E_POINTER;
    return _msg->get_Body( body );
}

//-------------------------------------------------------------------------------Com_mail::add_file

STDMETHODIMP Com_mail::add_file( BSTR filename, BSTR content_type )
{
    HRESULT hr = NOERROR;

    try
    {
        _msg->AddAttachment( filename, false, content_type );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

    return hr;
}

//-------------------------------------------------------------------------Com_mail::add_attachment
/*
STDMETHODIMP Com_mail::add_attachment( BSTR filename, BSTR content_type )
{
    HRESULT hr = NOERROR;

    try
    {
        _msg->AddAttachment( filename, content_type );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Mail.add_file" ); }

    return hr;
}
*/
//-----------------------------------------------------------------------------------Com_mail::send

void Com_mail::send()
{
    _msg->ClearRecipients();

    add_to_recipients( _to , 0 );
    add_to_recipients( _cc , 1 );
    add_to_recipients( _bcc, 2 );

    _msg->Send( _smtp_server, false );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

