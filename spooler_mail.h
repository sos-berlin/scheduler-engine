// $Id: spooler_mail.h,v 1.14 2003/03/21 11:22:56 jz Exp $

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
    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "spooler.Mail" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


    Z_GNU_ONLY(                 Com_mail                    ();  )                                  // Für gcc 3.2. Nicht implementiert
                                Com_mail                    ( Spooler* );
                               ~Com_mail                    ();
                                
    void                        init                        ();

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Mail", result ); }

    STDMETHODIMP            put_to                          ( BSTR );
    STDMETHODIMP            get_to                          ( BSTR* to )                            { *to = _to;  return NOERROR; }

    STDMETHODIMP            put_cc                          ( BSTR );
    STDMETHODIMP            get_cc                          ( BSTR* cc )                            { *cc = _cc;  return NOERROR; }

    STDMETHODIMP            put_bcc                         ( BSTR );
    STDMETHODIMP            get_bcc                         ( BSTR* bcc )                           { *bcc = _bcc; return NOERROR; }

    STDMETHODIMP            put_from                        ( BSTR );
    STDMETHODIMP            get_from                        ( BSTR* );
                                                                                                    
    STDMETHODIMP            put_subject                     ( BSTR );
    STDMETHODIMP            get_subject                     ( BSTR* );

    STDMETHODIMP            put_body                        ( BSTR );
    STDMETHODIMP            get_body                        ( BSTR* );

    STDMETHODIMP                add_file                    ( BSTR real_filename, BSTR mail_filename, BSTR content_type, BSTR encoding );
  //STDMETHODIMP                add_attachment              ( BSTR filename, BSTR data );

    STDMETHODIMP            put_smtp                        ( BSTR );
    STDMETHODIMP            get_smtp                        ( BSTR* );

    STDMETHODIMP            put_queue_dir                   ( BSTR );
    STDMETHODIMP            get_queue_dir                   ( BSTR* );

    STDMETHODIMP                add_header_field            ( BSTR field_name, BSTR value );

    STDMETHODIMP                dequeue                     ( int* count );

    STDMETHODIMP            get_dequeue_log                 ( BSTR* result )                        { return string_to_bstr( _msg->dequeue_log(), result ); }

    int                         auto_dequeue                ()                                      { return _msg->auto_dequeue(); }

    int                         send                        ();

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;

    Sos_ptr<mail::Message>     _msg;

    Bstr                       _subject;
    Bstr                       _from;
    Bstr                       _to;
    Bstr                       _cc;
    Bstr                       _bcc;
};

} //namespace spooler
} //namespace sos


#endif
