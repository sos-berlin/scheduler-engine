// $Id$

#ifndef __SPOOLER_MAIL_H
#define __SPOOLER_MAIL_H

#include <map>

//#include "../kram/olestd.h"
#include "../zschimmer/com.h"
#include "../kram/sysxcept.h"
#include "../kram/sosscrpt.h"
#include "../kram/com.h"
#include "../kram/com_server.h"
#include "../kram/sos_mail.h"


namespace sos {
namespace spooler {


//-----------------------------------------------------------------------------------------Com_mail

struct Com_mail : spooler_com::Imail, 
                  spooler_com::Ihas_java_class_name, 
                  Sos_ole_object               
{
    struct File
    {
        string                 _real_filename;
        string                 _mail_filename;
        string                 _content_type;
        string                 _encoding;
    };


    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "spooler.Mail" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


    Z_GNU_ONLY(                 Com_mail                    ();  )                                  // Für gcc 3.2. Nicht implementiert
                                Com_mail                    ( Spooler* );
                               ~Com_mail                    ();
                                
    void                        init                        ();

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Mail"; }

    STDMETHODIMP            put_To                          ( BSTR );
    STDMETHODIMP            get_To                          ( BSTR* result )                        { return String_to_bstr( _to, result ); }

    STDMETHODIMP            put_Cc                          ( BSTR );
    STDMETHODIMP            get_Cc                          ( BSTR* result )                        { return String_to_bstr( _cc, result ); }

    STDMETHODIMP            put_Bcc                         ( BSTR );
    STDMETHODIMP            get_Bcc                         ( BSTR* result )                        { return String_to_bstr( _bcc, result ); }

    STDMETHODIMP            put_From                        ( BSTR );
    STDMETHODIMP            get_From                        ( BSTR* );
                                                                                                    
    STDMETHODIMP            put_Subject                     ( BSTR );
    STDMETHODIMP            get_Subject                     ( BSTR* );

    STDMETHODIMP            put_Body                        ( BSTR );
    STDMETHODIMP            get_Body                        ( BSTR* );

    STDMETHODIMP                Add_file                    ( BSTR real_filename, BSTR mail_filename, BSTR content_type, BSTR encoding );
  //STDMETHODIMP                Add_attachment              ( BSTR filename, BSTR data );

    STDMETHODIMP            put_Smtp                        ( BSTR );
    STDMETHODIMP            get_Smtp                        ( BSTR* );

    STDMETHODIMP            put_Queue_dir                   ( BSTR );
    STDMETHODIMP            get_Queue_dir                   ( BSTR* );

    STDMETHODIMP                Add_header_field            ( BSTR field_name, BSTR value );

    STDMETHODIMP                Dequeue                     ( int* count );

    STDMETHODIMP            get_Dequeue_log                 ( BSTR* result )                        { return String_to_bstr( _msg->dequeue_log(), result ); }

    STDMETHODIMP            put_Xslt_stylesheet_path        ( BSTR path )                           { _xslt_stylesheet.release(); return Bstr_to_string( path, &_xslt_stylesheet_path ); }
    STDMETHODIMP            get_Xslt_stylesheet_path        ( BSTR* result )                        { return String_to_bstr( _xslt_stylesheet_path, result ); }
    STDMETHODIMP            get_Xslt_stylesheet             ( spooler_com::Ixslt_stylesheet** );


    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& dom );
    void                    set_dom                         ( const xml::Element_ptr& );

    void                    set_subject                     ( const string& );
    void                    set_from                        ( const string& );
    void                    set_from_name                   ( const string& );
    void                    set_to                          ( const string& );
    void                    set_cc                          ( const string& );
    void                    set_bcc                         ( const string& );
    void                    set_body                        ( const string& );
    void                    set_smtp                        ( const string& );
    string                      smtp                        ()                                      { return _smtp; }
    void                        add_header_field            ( const string& name, const string& value );
    void                        add_file                    ( const string& real_filename, const string& mail_filename, const string& content_type, const string& encoding );


    int                         auto_dequeue                ()                                      { return _msg->auto_dequeue(); }
    int                         send                        ();
    ptr<Xslt_stylesheet>        xslt_stylesheet             ();

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;

    Sos_ptr<mail::Message>     _msg;

    string                     _smtp;
    string                     _subject;
    string                     _from;
    string                     _to;
    string                     _cc;
    string                     _bcc;
    string                     _body;
    list<File>                 _files;

    typedef list< pair<string,string> >  Header_fields;
    Header_fields              _header_fields;

    string                     _xslt_stylesheet_path;
    ptr<Xslt_stylesheet>       _xslt_stylesheet;
};

} //namespace spooler
} //namespace sos


#endif
