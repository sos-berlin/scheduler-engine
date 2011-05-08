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
#include "sos_mail.h"
#include "sos_mail_jmail.h"
#include "com_simple_standards.h"
#include "sos_mail_java.h"
#include "../zschimmer/file.h"
#include "../zschimmer/java.h"


using zschimmer::vector_split;
using namespace zschimmer::javabridge;

namespace sos {
namespace mail {

using namespace std;

//-----------------------------------------------------------------------Java_message::Java_message

Java_message::Java_message( zschimmer::javabridge::Vm* vm )
:
    //Global_jobject(vm), 
    _zero_(this+1),
    _java_vm                    ( vm ),
    _java_class                 ( "sos/mail/Message" ),
    _close_method               ( &_java_class, "close"           , "()V" ),
    _set_property_method        ( &_java_class, "set_property"    , "(Ljava/lang/String;Ljava/lang/String;)V" ),
    _set_method                 ( &_java_class, "set"             , "(Ljava/lang/String;[B)V" ),
    _get_method                 ( &_java_class, "get"             , "(Ljava/lang/String;)Ljava/lang/String;" ),
    _add_header_field_method    ( &_java_class, "add_header_field", "(Ljava/lang/String;Ljava/lang/String;)V" ),
    _add_attachment_method      ( &_java_class, "add_attachment"  , "([BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" ),
    _add_file_method            ( &_java_class, "add_file"        , "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" ),
    _send_method                ( &_java_class, "send"            , "()V" )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );
    
    jobject jo = env->NewObject( _java_class, _java_class.method_id( "<init>", "()V" ) );
    if( !jo )  env.throw_java( "NewObject", _java_class._name );

    assign( jo );

    int needed_version = 2;
    env->CallVoidMethod( get_jobject(), z::javabridge::Method( &_java_class, "need_version", "(I)V" ), needed_version );
    if( env->ExceptionCheck() )  env.throw_java( "need_version" );
    
    if( _static->_debug )  set( "debug", "1" );

   
    if( sos_static_ptr()->_mail->_factory_ini_path != "" )      // Jira JS-136
    {
        for( Any_file f ( "-in profile -section=smtp " + sos_static_ptr()->_mail->_factory_ini_path ); !f.eof(); )        
        {
            Record        record  = f.get_record();
            Local_jstring name_j  ( record.as_string( "entry" ) );
            Local_jstring value_j ( record.as_string( "value" ) );

            env->CallVoidMethod( get_jobject(), _set_property_method, (jstring)name_j, (jstring)value_j );
            if( env->ExceptionCheck() )  env.throw_java( "set_property" );
        }
    }

    Message::init();
}

//----------------------------------------------------------------------Java_message::~Java_message

Java_message::~Java_message()
{
    // Das ist etwas krumm programmiert: _java_vm müssen wir _nach_ der Oberklasse Global_jobject freigeben.

    {
        Env env = jni_env();
        Local_frame local_frame ( 10 );

        env->CallVoidMethod( get_jobject(), _close_method );
        //if( _env->ExceptionCheck() )  _env.throw_java( "add_file" );

        Global_jobject::assign( NULL );     // Jetzt tut ~Global_jobject nichts mehr und die Java-VM kann freigeben werden.
    }
}

//-------------------------------------------------------------------------Java_message_object::set

void Java_message::set( const string& what, const char* value, int length )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    Local_jstring jwhat  ( what );
  //Local_jstring jvalue ( value, length );

    local_jobject<jbyteArray> byte_jarray ( env->NewByteArray( length ) );
    jbyte* byte_array = env->GetByteArrayElements( byte_jarray, NULL );
    memcpy( byte_array, value, length );
    env->ReleaseByteArrayElements( byte_jarray, byte_array, 0 );


    if( env.get_vm()->debug() )  LOG( "Java_message::set " << what << "=" << string(value,length) << "\n" );

    env->CallVoidMethod( get_jobject(), _set_method.id(), (jstring)jwhat, (jbyteArray)byte_jarray );
    if( env->ExceptionCheck() )  env.throw_java( "set", what + "=" + string(value,length) );
}

//-------------------------------------------------------------------------Java_message_object::get

string Java_message::get( const string& what, bool log_allowed )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    Local_jstring jwhat ( what );

    string result = env.string_from_jstring( (jstring)env->CallObjectMethod( get_jobject(), _get_method, (jstring)jwhat ) );

    if( log_allowed && env.get_vm()->debug() )  LOG( "Java_message::get " << what << "==>" << result << '\n' );
    if( env->ExceptionCheck() )  env.throw_java( "get", what );

    return result;
}

//---------------------------------------------------------------------------Java_message::set_from

void Java_message::set_from( const string& from )
{
    set( "from", from );
}

//-------------------------------------------------------------------------------Java_message::from

string Java_message::from()
{
    return get( "from" );
}

//-----------------------------------------------------------------------Java_message::set_reply_to

void Java_message::set_reply_to( const string& reply_to )
{
    set( "reply-to", reply_to );
}

//---------------------------------------------------------------------------Java_message::reply_to

string Java_message::reply_to()
{
    return get( "reply-to" );
}

//-----------------------------------------------------------------------Java_message::set_subject_

void Java_message::set_subject_( const string& subject )
{
    set( "subject", subject.substr( 0, 200 ) );
}

//----------------------------------------------------------------------------Java_message::subject

string Java_message::subject()
{
    return get( "subject" );
}

//---------------------------------------------------------------------------Java_message::set_body

void Java_message::set_body( const string& body )
{
    set( "body", body );
}

//-------------------------------------------------------------------------------Java_message::body

string Java_message::body()
{
    return get( "body" );
}

//-------------------------------------------------------------------Java_message::set_content_type

void Java_message::set_content_type( const string& content_type )
{
    //if( content_type != ""  &&  content_type != "text/plain" )  throw_xc( "Java_message::set_content_type" );
    set( "content_type", content_type );
}

//-----------------------------------------------------------------------Java_message::set_encoding

void Java_message::set_encoding( const string& encoding )
{
    //throw_xc( "Java_message::set_encoding" );
    //LOG( "Java_message::set_encoding(\"" << encoding << "\") wird für JavaMail ignoriert\n" );
    set( "encoding", encoding );
}

//--------------------------------------------------------------------------Java_message::add_file_

void Java_message::add_file_( const string& real_filename, const string& new_filename, const string& content_type, const string& encoding )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    Local_jstring real_filename_j ( real_filename     );
    Local_jstring new_filename_j  ( new_filename     );
    Local_jstring content_type_j  ( content_type.c_str() );     //content_type.empty()? "text/plain" : 
    Local_jstring encoding_j      ( encoding     );

    if( env.get_vm()->debug() )  LOG( "Java_message::add_file()\n" );

    env->CallVoidMethod( get_jobject(), _add_file_method, 
                         (jstring)real_filename_j, (jstring)new_filename_j, (jstring)content_type_j, (jstring)encoding_j );
    if( env->ExceptionCheck() )  env.throw_java( "add_file" );

/*
    try
    {
        zschimmer::Mapped_file file ( real_filename, "r" );

        add_attachment( file.map(), file.length(), 
                        mail_filename != ""? mail_filename : filename_of_path( real_filename ), 
                        content_type, encoding );
    }
    catch( const exception& x )  { throw_xc( x ); }
*/
}

//---------------------------------------------------------------------Java_message::add_attachment

void Java_message::add_attachment( const string& data, const string& filename, const string& content_type, const string& encoding )
{
    add_attachment( data.data(), data.length(), filename, content_type, encoding );
}

//---------------------------------------------------------------------Java_message::add_attachment

void Java_message::add_attachment( const void* data, int length, const string& filename, const string& content_type, const string& encoding )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    local_jobject<jbyteArray> byte_jarray ( env->NewByteArray( length ) );
    jbyte* byte_array = env->GetByteArrayElements( byte_jarray, NULL );
    memcpy( byte_array, data, length );
    env->ReleaseByteArrayElements( byte_jarray, byte_array, 0 );

    Local_jstring data_j         ( (const char*)data, length );
    Local_jstring filename_j     ( filename     );
    Local_jstring content_type_j ( content_type.c_str() );    //content_type.empty()? "text/plain" : 
    Local_jstring encoding_j     ( encoding     );

    if( env.get_vm()->debug() )  LOG( "Java_message::add_attachment()\n" );

    env->CallVoidMethod( get_jobject(), _add_attachment_method, 
                         (jbyteArray)byte_jarray, 
                         (jstring)filename_j, (jstring)content_type_j, (jstring)encoding_j );
    if( env->ExceptionCheck() )  env.throw_java( "add_attachment" );
}

//-------------------------------------------------------------------Java_message::set_header_field

void Java_message::add_header_field( const string& field_name, const string& value )
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    Local_jstring field_name_j ( field_name );
    Local_jstring value_j      ( value      );

    if( env.get_vm()->debug() )  LOG( "Java_message::add_header_field " << field_name << ": " << value << '\n' );

    env->CallVoidMethod( get_jobject(), _add_header_field_method, (jstring)field_name_j, (jstring)value_j );
    if( env->ExceptionCheck() )  env.throw_java( "add_header_field" );
}

//------------------------------------------------------------------------------Java_message::send2

void Java_message::send2()
{
    Env env = jni_env();
    Local_frame local_frame ( 10 );

    LOG( "JavaMail Send smtp=" << get("smtp",false) << " to=\"" << get("to",false) << "\" subject=\"" << get("subject",false) << "\"\n" );

    env->CallVoidMethod( get_jobject(), _send_method.id() );
    if( env->ExceptionCheck() )  env.throw_java( "send" );

    LOG( "JavaMail Send fertig\n" );
}

//------------------------------------------------------------------------Java_message::rfc822_text

string Java_message::rfc822_text()
{
    return get( "rfc822_text" );
}

//------------------------------------------------------------------------Java_message::send_rfc822

void Java_message::send_rfc822( const char* rfc822_text, int length )
{
    set( "send_rfc822", rfc822_text, length );
}

//-------------------------------------------------------------------------------------------------

} //namespace mail
} //namespace sos
