// $Id: spooler_mail.h,v 1.20 2004/04/05 08:49:46 jz Exp $

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

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Mail"; }

    STDMETHODIMP            put_To                          ( BSTR );
    STDMETHODIMP            get_To                          ( BSTR* to )                            { return _to.CopyTo( to ); }

    STDMETHODIMP            put_Cc                          ( BSTR );
    STDMETHODIMP            get_Cc                          ( BSTR* cc )                            { return _cc.CopyTo( cc ); }

    STDMETHODIMP            put_Bcc                         ( BSTR );
    STDMETHODIMP            get_Bcc                         ( BSTR* bcc )                           { return _bcc.CopyTo( bcc ); }

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
