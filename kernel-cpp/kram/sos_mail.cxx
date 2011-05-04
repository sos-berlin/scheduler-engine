// $Id$


#include "precomp.h"

#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef SYSTEM_WIN
#   include <io.h>
#   include <share.h>
#endif

#include "sos.h"
#include "sosprof.h"
#include "sos_java.h"
#include "sos_mail.h"
#include "sos_mail_java.h"
#include "../zschimmer/file.h"
#include "../zschimmer/z_mail.h"

#ifdef SYSTEM_WIN
#   include "sos_mail_jmail.h"
#endif



using zschimmer::file::z_filelength;
using zschimmer::vector_split;



namespace sos {

DEFINE_SOS_STATIC_PTR( mail::Mail_static );

namespace mail {

using namespace std;

//--------------------------------------------------------------------------------------------const

const int max_subject_length = 200;     // Möglicherweise gehen bis 1024-length("Subject: ") Zeichen?

//-------------------------------------------------------------------------------------------static

static z::Mutex                 dequeue_lock ( "email_dequeue" );   

//-----------------------------------------------------------------------------------create_message

Sos_ptr<Message> create_message( z::javabridge::Vm* java_vm, const string& type )
{
    string t = z::lcase( type );


    if( t == "" )
    {
#       ifdef SYSTEM_WIN
        {
            Sos_ptr<Jmail_message> message = SOS_NEW( Jmail_message( false ) );
            if( message->is_installed() )
            {
                message->init();
                return +message;
            }
        }
#       endif

        t = "java";
    }


#   ifdef SYSTEM_WIN
    if( t == "jmail" || t == "dimac" )
    {
        Sos_ptr<Jmail_message> message = SOS_NEW( Jmail_message );
        return +message;
    }
    else
#   endif
    if( t == "java" )
    {
        ptr<z::javabridge::Vm> vm = java_vm;
        
        if( !vm )  vm = get_java_vm();

        vm->start();     // Falls es vergessen worden ist.

        Sos_ptr<mail::Java_message> m = SOS_NEW( mail::Java_message( vm ) );
        return +m;
    }
    else
        throw_xc( "SOS-1451", type );

    return NULL;
}

//----------------------------------------------------------------------------------------make_addr
/*
string make_addr( const string& addr, const string& name )
{
    if( name.empty()  ||  addr == name ) 
    {
        return addr;
    }
    else
    {
        return name + " <" + addr + ">";
    }
}
*/
//---------------------------------------------------------------------------Email_addr::Email_addr
/*
Email_addr::Email_addr( const string& addr )
{
    const char* t = parse( addr.c_str() );
    if( *t )  throw_xc( "SOS-1442", addr );
}

//--------------------------------------------------------------------------------Email_addr::parse

const char* Email_addr::parse( const char* t )
{
    _name = "";
    _addr = "";

    if( !t )  return "";

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
*/
//----------------------------------------------------------------------------Mail_static::instance

Mail_static* Mail_static::instance()
{
    if( !sos_static_ptr()->_mail ) 
    {
        Sos_ptr<Mail_static> s = SOS_NEW( Mail_static );
        sos_static_ptr()->_mail = +s;
    }

    return +sos_static_ptr()->_mail;
}

//-------------------------------------------------------------------------Mail_static::Mail_static

Mail_static::Mail_static()
:
    _zero_(this+1)
{
#   ifdef _DEBUG
        //_debug = true;
#   endif

    _queue_dir         = read_profile_string( "", "mail", "queue_dir" );
    _smtp_server       = read_profile_string( "", "mail", "smtp"      );
    _from              = read_profile_string( "", "mail", "from"      );
    _cc                = read_profile_string( "", "mail", "cc"        );
    _bcc               = read_profile_string( "", "mail", "bcc"       );
    _iso_encode_header = read_profile_bool  ( "", "mail", "iso_encode_header", true );
    _auto_dequeue      = read_profile_bool  ( "", "mail", "auto_dequeue"     , false );
    _queue_only        = read_profile_bool  ( "", "mail", "queue_only"       , false );
    _debug             = read_profile_bool  ( "", "mail", "debug"            , false );
}

//----------------------------------------------------------------------Mail_static::read_smtp_user
    
void Mail_static::read_smtp_user()
{
    if( !_smtp_username_set  &&  _factory_ini_path != "" )
    {
        _smtp_username = read_profile_string( _factory_ini_path, "smtp", "mail.smtp.user"     );
        _smtp_password = read_profile_string( _factory_ini_path, "smtp", "mail.smtp.password" );
        _smtp_username_set = true;
    }
}

//---------------------------------------------------------------------------------Message::Message

Message::Message()
:
    _zero_(this+1)
{
    _static = Mail_static::instance();;

    _queue_only = _static->_queue_only;
}

//--------------------------------------------------------------------------------Message::~Message
/*
Message::~Message()
{
}
*/
//------------------------------------------------------------------------------------Message::init

void Message::init()
{
    set_queue_dir( _static->_queue_dir   );
    // set_smtp     ( _static->_smtp_server );  // JS-544
    if (!_static->_smtp_server.empty() )        // JS-544
        set_smtp     ( _static->_smtp_server ); // JS-544
    set_from     ( _static->_from        );
    set_cc       ( _static->_cc          );
    set_bcc      ( _static->_bcc         );
}

//---------------------------------------------------------------------------Message::set_from_name

void Message::set_from_name( const string& name )
{
    zschimmer::Email_address a ( from() );
    
    a.set_name( name );
    set_from( a );
/*
    string old_from = this->from();

    if( old_from.find( '<' ) == string::npos  &&  old_from.find( '@' ) != string::npos )
    {
        set_from( '"' + name + "\" <" + old_from + ">" );
    }
*/
}

//-----------------------------------------------------------------------------Message::set_subject

void Message::set_subject( const string& subject )
{
    string s;
    s.reserve( max_subject_length );

    for( const char* p = subject.c_str(); s.length() < max_subject_length; )
    {
        if( (Byte)p[0] < ' ' )  break;                  // Vor allem \0, \r und \n
        if( p[0] == ' ' )  while( p[1] == ' ' )  p++;   // Plumpe Spam-Filter mögen keine aufeinanderfolgenden Blanks

        s += p[0];
        p++;
    }

    set_subject_( s );
}

//--------------------------------------------------------------------------------Message::add_file

void Message::add_file( const string& real_filename, const string& mail_filename, const string& content_type, const string& encoding )
{
    //if( real_filename.find( '|' ) )     // Hostware-Dateiname?
    //{
/*
        int increment = 10*1024*1024;
        
        Dynamic_area buffer ( 1024*1024 );
        Any_file     file   ( "-in -binary " + real_filename);
        
        while(1)
        {
            try
            {
                if( buffer.rest().length() == 0 )  buffer.resize_min( buffer.size() + increment );
                file.get( buffer.rest() );
            }
            catch( const Eof_error& ) { break; }
        }
*/
    //    add_attachment( file_as_string( real_filename ), mail_filename, content_type, encoding );
    //}
    //else
    {
        add_file_( real_filename, mail_filename, content_type, encoding );
    }
}

//------------------------------------------------------------------------------------Message::send

bool Message::send()
{
    //LOG( "Message::send  _queue_dir=" << _queue_dir << "\n" );

    if( _queue_only )   //  ||  smtp() == "-queue" )
    {
        enqueue();
    }
    else
    if( _queue_dir.empty() )
    {
        send2();
    }
    else
    {
        try
        {
            send2();
        }
        catch( const exception& x )  
        { 
            LOG( "email send: " << x << '\n'; );  
            enqueue(); 
            return false; 
        }
    }

    return true;
}

//---------------------------------------------------------------------------------Message::enqueue

void Message::enqueue()
{
    string     email_filename;
    char       filename_buffer [100];
    int        file   = -1;
    string     text;
    timeb      tm;      ftime( &tm );
    struct tm* t      = localtime( &tm.time );
    const  string ext = ".email";

    try
    {
        text = rfc822_text();
    }
    catch( const _com_error& x )  { throw_com_error(x,"mail"); }
    catch( const exception& x )   { throw_xc(x); }


    zschimmer::z_snprintf( filename_buffer, sizeof filename_buffer,
                           "/sos.%04d-%02d-%02d.%02d%02d%02d.%03d", 
                           1900+t->tm_year, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tm.millitm );
    string filename1 = filename_buffer;

    for( int i = 0;; i++ )
    {
        email_filename = _queue_dir + filename1;
        if( i > 0 )  email_filename += "." + as_string(i);
        email_filename += ext + "~";

#       ifdef Z_WINDOWS        
            file = sopen( email_filename.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_BINARY | _O_SEQUENTIAL | O_NOINHERIT, _SH_DENYRW, 0600 );
#       else
            file = open( email_filename.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_BINARY | O_NOINHERIT, 0600 );
#       endif

        if( file != -1 )  break;
        if( errno != EEXIST )  throw_errno( errno, email_filename.c_str() );
    }

    LOG( "eMail wird gespeichert in " << email_filename << "\n" );

    //string to = "To: " + this->to() + "\r\n";
    //write( file, to.c_str(), to.length() );

    int ret = write( file, text.c_str(), text.length() );
    if( ret != text.length() )  close(file), throw_errno( errno, email_filename.c_str() );

    ret = close( file );
    if( ret == -1 )  throw_errno( errno, email_filename.c_str() );


    for( int i = 0; i < 1000; i++ )
    {
        string filename = email_filename.substr( 0, email_filename.length() - ( i == 0? 1 : ext.length() + 1 ) );      // "~" oder ".email~" abschneiden
        if( i > 0 )  filename += "." + as_string(i) + ext;

#       ifdef Z_UNIX
            if( zschimmer::file::File_path( filename ).file_exists() )  continue;
#       endif

        int err = rename( email_filename.c_str(), filename.c_str() );
        if( err  &&  errno == EEXIST )  continue;
        if( err )  throw_errno( errno, "rename", filename.c_str() );

        LOG( "eMail-Datei umbenannt in  " << filename << "\n" );
        break;
    }
}


//-----------------------------------------------------------------------------Message::dequeue_log

void Message::dequeue_log( const string& line )
{
    LOG( line << '\n' );

    _dequeue_log += Sos_optional_date_time::now().as_string();
    _dequeue_log += ' ';
    _dequeue_log += line;
    _dequeue_log += '\n';
}

//---------------------------------------------------------------------------------Message::dequeue

int Message::dequeue()
{
    bool    empty_dir = false;
    int     count = 0;
    string  filename;
    int     file = -1;

    if( _queue_dir.empty() )  return count;

    Z_MUTEX( dequeue_lock )
    try
    {
        _dequeue_log = "";

        while(1)
        {
            filename = "";

            if( _static->_dir.opened()  &&  _static->_dir.eof() )  _static->_dir.close(); 

            if( !_static->_dir.opened() ) 
            {
                if( empty_dir )  break;

                LOG( "Message::dequeue " << _queue_dir << "\n" );

#               if defined __GNUC__ && __GNUC_VERSION_ < 30202  // Bis gcc 3.2.1:
                    _static->_dir.open( "-in dir " + _queue_dir + "/sos.*.email" );
#                else
                    _static->_dir.open( "-in select filename order by filename | dir " + _queue_dir + "/sos.*.email" );
#               endif

                if( _static->_dir.eof() )  { _static->_dir.close();  break; }
                
                empty_dir = true;
            }

            Dynamic_area record;
            _static->_dir.get( &record );

            filename = _static->_dir.record_type()->as_string( "filename", record.byte_ptr() );

            string email_filename = _queue_dir + "/" + filename;

            //LOGI( "open " << email_filename << "\n" );

#           ifdef Z_WINDOWS
                file = sopen( email_filename.c_str(), O_RDONLY | _O_BINARY | _O_SEQUENTIAL | O_NOINHERIT, _SH_DENYRW );   // Exklusiv Öffnen, denn mehrere Threads und Prozesse (auch Spooler & hostole) können dasselbe Verzeichnis lesen
#           else
                file = open( email_filename.c_str(), O_RDONLY | O_NOINHERIT );
#           endif

            if( file == -1 ) {
                if( errno == ENOENT ) { dequeue_log( "Message::dequeue: Jemand hat " + email_filename + " gelöscht" );  continue; }
                if( errno == EACCES ) { dequeue_log( "Message::dequeue: Jemand sperrt " + email_filename );  continue; }
                else throw_errno( errno, email_filename.c_str() );
            }

            
            dequeue_log( "Message::dequeue: Versende aus Warteschlange " + email_filename );

            
            int size = z_filelength( file );
            if( size < 0 )  throw_errno( errno, email_filename.c_str() );

            Dynamic_area text ( size+1 );
            int length = read( file, text.ptr(), size );

            send_rfc822( text.char_ptr(), size );

            close( file );

            int ret = unlink( email_filename.c_str() );
            if( ret == -1 )  throw_errno( errno, email_filename.c_str() );

            count++;
            empty_dir = false;
        }
    }
    catch( const exception& x )
    {
        _dequeue_log += x.what();
        _dequeue_log += '\n';

        if( file != -1 )  close( file );
        Xc xc = x;
        xc.insert( filename );
        throw xc;
    }

    return count;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
} //namespace mail
