// $Id$

#ifndef __SOS_MAIL_JMAIL_H
#define __SOS_MAIL_JMAIL_H
#ifdef SYSTEM_WIN

#include <map>
#include "sysxcept.h"
#include "../file/anyfile.h"
#include "com_simple_standards.h"

//#import "../misc/lib/jmail.tlb"     // eMail-Komponente von Dimac AB, www.dimac.se
#import "../3rd_party/dimac/jmail.dll"     // eMail-Komponente von Dimac AB, www.dimac.se
#include "../zschimmer/regex_class.h"
#include "sos_mail.h"


namespace sos {
namespace mail {

//------------------------------------------------------------------------------------Jmail_message

struct Jmail_message : Message
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


                                Jmail_message               ( bool call_init = true );
                               ~Jmail_message               ();
                               
    void                        init                        ();
    bool                        is_installed                ()                                      { return SUCCEEDED( _create_instance_hr ); }

    void                    set_to                          ( const string& );
    string                      to                          ()                                      { return _to; }

    void                    set_cc                          ( const string& );
    string                      cc                          ()                                      { return _cc; }

    void                    set_bcc                         ( const string& );
    string                      bcc                         ()                                      { return _bcc; }

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
    void                        add_header_field            ( const string& field_name, const string& value );

    void                    set_smtp                        ( const string& smtp )                  { _smtp_server = smtp; }
    string                      smtp                        ()                                      { return _smtp_server; }

    bool                        send                        ();
    void                        send2                       ();

    string                      rfc822_text                 ();
    void                        send_rfc822                 ( const char* rfc822_text, int length );

  private:
    void                        add_custom_attachment       ( BSTR data, const string& filename, const string& content_type = "" );
    void                        add_to_recipients           ( const string& recipients, char recipient_type );
    string                      get_recipients              ();
    static std::vector<string>  split_addresslist           ( const string& address_list )  { return zschimmer::vector_split( " *, *", address_list ); }


    Fill_zero                  _zero_;
    Ole_initialize             _ole;

    string                     _to;
    string                     _cc;
    string                     _bcc;
    string                     _from;
    string                     _subject;
  //list<Header_field>         _header_fields;
  //list<Attachment>           _files;
    string                     _body;
    string                     _smtp_server;
    string                     _content_type;
    bool                       _attachment_added;

    jmail::IMessagePtr         _msg;
    HRESULT                    _create_instance_hr;

};

} //namespace mail
} //namespace sos


#endif
#endif
