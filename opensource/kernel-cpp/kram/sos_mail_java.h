// $Id: sos_mail_java.h 13870 2010-06-03 15:29:33Z jz $

#ifndef __SOS_MAIL_JAVA_H
#define __SOS_MAIL_JAVA_H

#include <map>
#include "sysxcept.h"
#include "../file/anyfile.h"
#include "com_simple_standards.h"

#include "../zschimmer/regex_class.h"
#include "../zschimmer/java.h"


namespace sos {
namespace mail {

//-------------------------------------------------------------------------------------Java_message

struct Java_message : Message, z::javabridge::Global_jobject
{
/*
    struct Attachment
    {
        string                 _filename;
        string                 _data;
        bool                   _has_data;
        string                 _content_type;
    };


    struct Header_field
    {
        string                 _name;
        string                 _value;
    };
*/


                                Java_message                ( zschimmer::javabridge::Vm* );
                               ~Java_message                ();
                                
  //void                        init                        ();

    void                    set_to                          ( const string& to )                    { set( "to", to ); }
    string                      to                          ()                                      { return get( "to" ); }

    void                    set_cc                          ( const string& cc )                    { set( "cc", cc ); }
    string                      cc                          ()                                      { return get( "cc" ); }

    void                    set_bcc                         ( const string& bcc )                   { set( "bcc", bcc ); }
    string                      bcc                         ()                                      { return get( "bcc" ); }

    void                    set_from                        ( const string& );
    string                      from                        ();

    void                    set_reply_to                    ( const string& );
    string                      reply_to                    ();

    void                    set_subject_                    ( const string& );
    string                      subject                     ();

    void                    set_body                        ( const string& );
    string                      body                        ();

    void                    set_content_type                ( const string& );
    void                    set_encoding                    ( const string& );

    void                        add_file_                   ( const string& real_filename, const string& mail_filename = "", const string& content_type = "", const string& encoding = "base64" );
    void                        add_attachment              ( const string& data, const string& filename, const string& content_type = "", const string& encoding = "base64" );
    void                        add_attachment              ( const void* data, int length, const string& filename, const string& content_type, const string& encoding );
    void                        add_header_field            ( const string& field_name, const string& value );

    void                    set_smtp                        ( const string& smtp )                  { _smtp = smtp; set( "smtp", smtp ); }
    string                      smtp                        ()                                      { return z::string_begins_with( _smtp, "-" )? _smtp : get( "smtp" ); }

    void                        send2                       ();

    string                      rfc822_text                 ();
    void                        send_rfc822                 ( const char* rfc822_text, int length );

  private:
    void                        set                         ( const string& what, const string& value )   { set( what, value.data(), value.length() ); }
    void                        set                         ( const string& what, const char* value, int length );
    string                      get                         ( const string& what, bool log_allowed = true );


    Fill_zero                  _zero_;

    ptr<zschimmer::javabridge::Vm>  _java_vm;                    // Muss vor allen Java-Objekten kommen, damit's als letztes freigeben wird!
    z::javabridge::Class       _java_class;
    z::javabridge::Method      _set_method;
    z::javabridge::Method      _get_method;
    z::javabridge::Method      _set_property_method;
    z::javabridge::Method      _add_header_field_method;
    z::javabridge::Method      _add_attachment_method;
    z::javabridge::Method      _add_file_method;
    z::javabridge::Method      _send_method;
    z::javabridge::Method      _close_method;
    string                     _smtp;
};

//-------------------------------------------------------------------------------------------------

} //namespace mail
} //namespace sos

#endif
