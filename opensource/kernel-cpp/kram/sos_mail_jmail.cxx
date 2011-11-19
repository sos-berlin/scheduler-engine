// $Id: sos_mail_jmail.cxx 13154 2007-11-03 07:11:44Z jz $


#include "precomp.h"

#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>

#include "sos.h"
#include "sosprof.h"
#include "sos_mail.h"
#include "sos_mail_jmail.h"
#include "com_simple_standards.h"
#include "../zschimmer/z_windows.h"
#include "../zschimmer/z_mail.h"


using zschimmer::vector_split;
using namespace zschimmer::windows;

namespace sos {
namespace mail {

using namespace std;

//-------------------------------------------------------------------------------------------static

//static _bstr_t                  empty_bstr;

//-------------------------------------------------------------------------Mail_static::Mail_static
/*
Mail_static::Mail_static()
{
    _queue_dir         = read_profile_string( "", "mail", "queue_dir" );
    _smtp_server       = read_profile_string( "", "mail", "smtp"      );
    _from              = read_profile_string( "", "mail", "from"      );
    _cc                = read_profile_string( "", "mail", "cc"        );
    _bcc               = read_profile_string( "", "mail", "bcc"       );
    _iso_encode_header = read_profile_bool  ( "", "mail", "iso_encode_header", true );
    _auto_dequeue      = read_profile_bool  ( "", "mail", "auto_dequeue", false );
}
*/

//-----------------------------------------------------------------------------------change_slashes

static string change_slashes( const string& filename )
{
#   ifdef Z_UNIX

        return filename;

#   else

        string result = filename;

        for( int i = 0; i < result.length(); i++ )  if( result[i] == '/' )  result[i] = '\\';

        return result;

#   endif
}

//---------------------------------------------------------------------Jmail_message::Jmail_message

Jmail_message::Jmail_message( bool call_init )
:
    _zero_(this+1)
{
    LOG( "CreateInstance(jmail::Message " << __uuidof( jmail::Message ) << ")\n" );
    _create_instance_hr = _msg.CreateInstance( __uuidof( jmail::Message ), NULL );
    if( call_init )  init();
}

//--------------------------------------------------------------------Jmail_message::~Jmail_message

Jmail_message::~Jmail_message()
{
    _msg = NULL;
}

//------------------------------------------------------------------------------Jmail_message::init

void Jmail_message::init()
{
    if( FAILED(_create_instance_hr) )  throw_ole( _create_instance_hr, "CoCreateInstance", "jmail::Message" );

    try
    {
        //if( _msg == NULL )
        //{
            //hr = _msg.CreateInstance( __uuidof( jmail::Message ), NULL );
            //if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", "jmail::Message" );

            // Jmail braucht 1s pro 100KB dafür:  _msg->Encoding = "quoted-printable";  // Für Anhänge

            // Jmail codiert sonst alle Header-Einträge mit Mime, auch wenn nur Ascii-Zeichen vorkommen. Und Microsoft Outlook zeigt sonst die Attachments nicht.
            _msg->ISOEncodeHeaders = _static->_iso_encode_header;     
        //}
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception&  x )  { throw_xc(x); }


    Message::init();
}

//-----------------------------------------------------------------Jmail_message::add_to_recipients

void Jmail_message::add_to_recipients( const string& recipients, char recipient_type )
{
    vector<string> addr_vector = split_addresslist( recipients );
    Z_FOR_EACH( vector<string>, addr_vector, it )
    {
        zschimmer::Email_address a = *it;
        switch( recipient_type )
        {
            case 0: _msg->AddRecipient   ( a.address().c_str(), a.name().c_str(), "" );  break;
            case 1: _msg->AddRecipientCC ( a.address().c_str(), a.name().c_str(), "" );  break;
            case 2: _msg->AddRecipientBCC( a.address().c_str(), a.name().c_str()     );  break;
        };
    }
}

//--------------------------------------------------------------------Jmail_message::get_recipients

string Jmail_message::get_recipients()
{
    string result;

    try
    {
        jmail::IRecipientsPtr recipients = _msg->Recipients;
        int n = recipients->Count;
    
        for( int i = 0; i < n; i++ )
        {
            if( i > 0 )  result += ", ";
            jmail::IRecipientPtr r = recipients->Item[i];

            zschimmer::Email_address a;
            a.set_address( bstr_as_string( r->EMail ) );
            if( a.address() == "unknown" )  a.set_address( "" );
            a.set_name( bstr_as_string( r->Name ) );
            result += a;
        }

    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }

    return result.c_str();
}

//----------------------------------------------------------------------------Jmail_message::set_to

void Jmail_message::set_to( const string& to )
{
    _to = to;
    if( to.empty() )  return;

    try
    {
        _msg->ClearRecipients();

        add_to_recipients( to, 0 );
        _to = get_recipients();
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//----------------------------------------------------------------------------Jmail_message::set_cc

void Jmail_message::set_cc( const string& cc )
{
    try
    {
        _cc = "";
        if( cc.empty() )  return;

        _msg->ClearRecipients();
        add_to_recipients( cc, 1 );
        _cc = get_recipients();
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//---------------------------------------------------------------------------Jmail_message::set_bcc

void Jmail_message::set_bcc( const string& bcc )
{
    _bcc = "";
    if( bcc.empty() )  return;

    try
    {
        _msg->ClearRecipients();
        add_to_recipients( bcc, 0 );
        _bcc = get_recipients();
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//--------------------------------------------------------------------------Jmail_message::set_from

void Jmail_message::set_from( const string& from )
{
    if( from.empty() )  return;

    zschimmer::Email_address addr = from;
    
    try
    {
        _msg->From     = addr.address().c_str();
        _msg->FromName = addr.name().c_str();
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//------------------------------------------------------------------------------Jmail_message::from

string Jmail_message::from()
{
    zschimmer::Email_address from;

    from.set_address( bstr_as_string( _msg->From ) );
    if( from.address() == "unknown" )  from.set_address( "" );

    from.set_name( bstr_as_string( _msg->FromName ) );

    return from;
}

//-----------------------------------------------------------------------Jmail_message::set_reply_to

void Jmail_message::set_reply_to( const string& reply_to )
{
    if( reply_to.empty() )  return;

    zschimmer::Email_address addr = reply_to;
    
    try
    {
        _msg->ReplyTo     = addr.address().c_str();
      //_msg->ReplyToName = addr.name().c_str();
    }
    catch( const _com_error& x )  { throw_com_error(x,"Jmail_message::set_reply_to"); }
    catch( const exception& x )   { throw_xc(x); }
}

//--------------------------------------------------------------------------Jmail_message::reply_to

string Jmail_message::reply_to()
{
    //return make_addr( bstr_as_string( _msg->ReplyTo ), "" );  //, bstr_as_string( _msg->ReplyToName ) );
    return bstr_as_string( _msg->ReplyTo );
}

//----------------------------------------------------------------------Jmail_message::set_subject_

void Jmail_message::set_subject_( const string& subject )
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );

    _subject = subject.substr(0,200);   // JMail verträgt keine zu langen Betreffs
/*
    int pos = _subject.find( '\n' );
    if( pos != string::npos )  _subject.erase( pos );
    if( _subject.length() > 0  &&  _subject[ _subject.length()-1 ] == '\r' )  _subject.erase( _subject.length()-1 );
*/
    try
    {
        _msg->put_Subject( Bstr( _subject.c_str() ) );
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//---------------------------------------------------------------------------Jmail_message::subject

string Jmail_message::subject()
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );
    
    return bstr_as_string( _msg->Subject );
}

//--------------------------------------------------------------------------Jmail_message::set_body

void Jmail_message::set_body( const string& body )
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );

    _msg->Body = body.c_str();
}

//------------------------------------------------------------------------------Jmail_message::body

string Jmail_message::body()
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );
    return bstr_as_string( _msg->Body );
}

//------------------------------------------------------------------Jmail_message::set_content_type

void Jmail_message::set_content_type( const string& content_type )
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );

    _content_type = content_type.c_str();
    //_msg->ContentType = content_type.c_str();
    //_msg->AddNativeHeader( "Content-Type", content_type.c_str() );
}

//----------------------------------------------------------------------Jmail_message::set_encoding

void Jmail_message::set_encoding( const string& encoding )
{
    if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );
    _msg->Encoding = encoding.c_str();
}

//-------------------------------------------------------------------------Jmail_message::add_file_

void Jmail_message::add_file_( const string& real_filename, const string& mail_filename, const string& content_type, const string& encoding )
{
    try
    {
        if( encoding != "" )  _msg->Encoding = encoding.c_str();

        string fn = mail_filename != ""? mail_filename : filename_of_path( real_filename );
        if( fn == filename_of_path( real_filename ) )
        {
            _msg->AddAttachment( change_slashes( real_filename ).c_str(), false, content_type.c_str() );
        }
        else
        {
            Handle file, file_map;
            void*  p = NULL;

            file = CreateFile( real_filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );   if( !file )  throw_mswin_error( "OpenFile", real_filename );

            int size = GetFileSize(file,NULL);

            if( size > 0 )
            {
                file_map = CreateFileMapping( file, NULL, PAGE_READONLY, 0, size, NULL );         if( !file_map )  throw_mswin_error( "CreateFileMapping", real_filename );
                p = MapViewOfFile( file_map, FILE_MAP_READ, 0, 0, size );                         if( !p )  throw_mswin_error( "MapViewOfFile" );
            }
            
            _bstr_t text ( SysAllocStringLen_char( (const char*)p, size ), false );

            if( p )  UnmapViewOfFile( p );
            file_map.close();
            file.close();

            _msg->AddCustomAttachment( change_slashes( fn ).c_str(), text, false );
            //add_custom_attachment( text, fn, content_type );
        }

        _attachment_added = true;
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//--------------------------------------------------------------------Jmail_message::add_attachment

void Jmail_message::add_attachment( const string& data, const string& filename, const string& /*content_type*/, const string& encoding )
{
    try
    {
        if( encoding != "" )  _msg->Encoding = encoding.c_str();
        //content_type kann nicht gesetzt werden?
        _msg->AddCustomAttachment( change_slashes( filename ).c_str(), _bstr_t( SysAllocString_string( data ), false ), false );
        //add_custom_attachment( _bstr_t( SysAllocString_string( data ), false ), filename, content_type );

        _attachment_added = true;
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//-------------------------------------------------------------Jmail_message::add_custom_attachment
/*
void Jmail_message::add_custom_attachment( BSTR data, const string& filename, const string& content_type )
{
    HRESULT                 hr;
    jmail::IAttachmentPtr   att;

    hr = att.CreateInstance( __uuidof(jmail::Attachment), NULL );
    if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", "jmail::Attachment" );

    jmail::IAttachmentPtr attachment = att->New( filename.c_str(), content_type.c_str(), data );

    _msg->Attachments->Add( &attachment );      // ABSTURZ
}
*/
//------------------------------------------------------------------Jmail_message::set_header_field

void Jmail_message::add_header_field( const string& field_name, const string& value )
{
    try
    {
        HRESULT hr;
        if( _msg == 0 )  throw_ole( E_POINTER, "Jmail_message" );

        hr = _msg->AddNativeHeader( field_name.c_str(), value.c_str() );
        if( FAILED(hr) )  throw_ole( hr, "AddNativeHeader" );
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//-----------------------------------------------------------------------------Jmail_message::send2

void Jmail_message::send2()
{
    try
    {
        _msg->ClearRecipients();

        add_to_recipients( _to , 0 );
        add_to_recipients( _cc , 1 );
        add_to_recipients( _bcc, 2 );

        if( !_content_type.empty() )
        {
            if( _attachment_added  )  LOG( "Content-Type wird nicht gesetzt, weil ein Anhang hinzugefügt ist und JMail die eMail dann unlesbar machen würde.\n " );
                                else  _msg->ContentType = _content_type.c_str();
        }

        Mail_static* m = Mail_static::instance();
        m->read_smtp_user();

        string server         = m->_smtp_server;
        string server_for_log = m->_smtp_server;

        //COM-8000FFFF  Schwerwiegender Fehler [The message was undeliverable. All servers failed to receive the message] [jmail]
        if( m->_smtp_username != "" )     
        {
            server         = m->_smtp_username + ":" + m->_smtp_password + "@" + server;
            server_for_log = m->_smtp_username + ":"   "???"               "@" + server_for_log;
        }

        LOG( "Jmail Send smtp=" << server_for_log << " to=\"" << _to << "\" subject=\"" << _subject << "\"\n" );
        _msg->Send( server.c_str(), false );
        LOG( "Jmail Send fertig\n" );
    }
    catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//-----------------------------------------------------------------------Jmail_message::rfc822_text

string Jmail_message::rfc822_text()
{
    return bstr_as_string( _msg->MailData );
}

//-----------------------------------------------------------------------Jmail_message::send_rfc822

void Jmail_message::send_rfc822( const char* rfc822_text, int length )
{
    try
    {
        jmail::IMessagePtr msg;
        HRESULT hr = msg.CreateInstance( __uuidof( jmail::Message ), NULL );
        if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", "jmail::Message" );

        const char* p     = rfc822_text;
        const char* p_end = p + length;

        if( length > 4  &&  memcmp( p, "To: ", 4 ) == 0 )           // 1. Zeile ist unsere "To:"-Zeile. Jmail will die Empfänger extra haben
        {
            p += 4;
            while( p[0] != '\0'  &&  p[0] != '\r'  &&  p[0] != '\n' )  p++;
            string to ( rfc822_text + 4, p - rfc822_text - 4 );
            if( p[0] == '\r' )  p++;
            if( p[0] == '\n' )  p++;

            add_to_recipients( to, 0 );
        }

        Bstr text_bstr ( p, rfc822_text + length - p );

        msg->ParseMessage( (BSTR)text_bstr );
        msg->Send( _smtp_server.c_str(), false );
    }
    catch( const exception& x )
    {
        _dequeue_log += x.what();
        _dequeue_log += '\n';

        throw_xc( x );
    }
    catch( _com_error& x )
    {
        _dequeue_log += bstr_as_string( x.Description() );
        _dequeue_log += '\n';

        throw_com_error( x );
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace mail
} //namespace sos
