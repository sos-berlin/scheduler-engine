// $Id: sos_mail.h 13870 2010-06-03 15:29:33Z jz $

#ifndef __SOS_MAIL_H
#define __SOS_MAIL_H

#include "../file/anyfile.h"

namespace sos {
namespace mail {

//--------------------------------------------------------------------------------------Mail_static

struct Mail_static : Sos_self_deleting
{
    static Mail_static*         instance                ();


                                Mail_static             ();

    void                        read_smtp_user          ();
    void                    set_factory_ini_path        ( const string& path )                      { _factory_ini_path = path; }

    Fill_zero                  _zero_;
    string                     _factory_ini_path;
    string                     _queue_dir;
    string                     _smtp_server;
    bool                       _smtp_username_set;
    string                     _smtp_username;          // Nur für sos_mail_jmail.cxx, soll aus factory.ini [smtp] gelesen werden
    string                     _smtp_password;
    string                     _cc;
    string                     _bcc;
    string                     _from;
    bool                       _iso_encode_header;
    bool                       _auto_dequeue;
    bool                       _queue_only;
    bool                       _debug;

    Any_file                   _dir;
};

//------------------------------------------------------------------------------------------Message

struct Message : Sos_self_deleting
{
                                Message                     ();

    virtual void                init                        ();

    virtual void            set_to                          ( const string& )                       = 0;
    virtual string              to                          ()                                      = 0;

    virtual void            set_cc                          ( const string& )                       = 0;
    virtual string              cc                          ()                                      = 0;

    virtual void            set_bcc                         ( const string& )                       = 0;
    virtual string              bcc                         ()                                      = 0;

    virtual void            set_from                        ( const string& )                       = 0;
    virtual string              from                        ()                                      = 0;
    virtual void            set_from_name                   ( const string& );

    virtual void            set_reply_to                    ( const string& )                       = 0;
    virtual string              reply_to                    ()                                      = 0;

            void            set_subject                     ( const string& );
    virtual void            set_subject_                    ( const string& )                       = 0;
    virtual string              subject                     ()                                      = 0;

    virtual void            set_body                        ( const string& )                       = 0;
    virtual string              body                        ()                                      = 0;

    virtual void            set_content_type                ( const string& )                       = 0;
    virtual void            set_encoding                    ( const string& )                       = 0;

            void                add_file                    ( const string& real_filename, const string& mail_filename = "", const string& content_type = "", const string& encoding = "base64" );
    virtual void                add_file_                   ( const string& real_filename, const string& mail_filename, const string& content_type, const string& encoding  ) = 0;
    virtual void                add_attachment              ( const string& data, const string& filename, const string& content_type = "", const string& encoding = "base64" ) = 0;
    
    virtual void                add_header_field            ( const string& field_name, const string& value ) = 0;

    virtual void            set_smtp                        ( const string& smtp )                  = 0;
    virtual string              smtp                        ()                                      = 0;

    void                    set_queue_only                  ( bool b )                              { _queue_only = b; }

    void                    set_queue_dir                   ( const string& dir )                   { _queue_dir = dir; }
    string                      queue_dir                   () const                                { return _queue_dir; }

    bool                        send                        ();
    virtual void                send2                       ()                                      = 0;

    virtual string              rfc822_text                 ()                                      = 0;
    virtual void                send_rfc822                 ( const char* rfc822_text, int length ) = 0;

    void                        enqueue                     ();
    int                         dequeue                     ();
    int                         auto_dequeue                ()                                      { _dequeue_log = ""; return  _static->_auto_dequeue? dequeue() : 0; }
    void                        dequeue_log                 ( const string& line );
    string                      dequeue_log                 () const                                { return _dequeue_log; }

    Fill_zero                  _zero_;
    Sos_ptr<Mail_static>       _static;
    string                     _queue_dir;                  // Verzeichnis für eMail, die wegen Fehler nicht verschickt werden kann
    string                     _dequeue_log;
    bool                       _queue_only;
};

//-------------------------------------------------------------------------------------------------

//string                        make_addr                   ( const string& addr, const string& name );
Sos_ptr<Message>                create_message              ( z::javabridge::Vm* = NULL, const string& type = "" );

//-------------------------------------------------------------------------------------------------

} //namespace mail
} //namespace sos

//-------------------------------------------------------------------------------------------------
/*
#ifdef SYSTEM_WIN

#   include "sos_mail_jmail.h"

    namespace sos 
    {
        typedef Jmail_message       Mail_message;
    }

#else

#   include "sos_mail_java.h"

    namespace sos 
    {
        typedef Java_mail_message   Mail_message;
    }

#endif
*/
//-------------------------------------------------------------------------------------------------

#endif
