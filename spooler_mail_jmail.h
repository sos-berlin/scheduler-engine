// $Id: spooler_mail_jmail.h,v 1.7 2002/04/07 11:47:23 jz Exp $

#ifndef __SPOOLER_MAIL_JMAIL_H
#define __SPOOLER_MAIL_JMAIL_H
#ifdef SYSTEM_WIN

#include <map>
#include "../kram/olestd.h"
#include "../kram/sysxcept.h"
#include "../kram/sosscrpt.h"
#include "../kram/com.h"
#include "../kram/com_server.h"

#include <atlbase.h>

//#import "../misc/lib/jmail.tlb"     // eMail-Komponente von Dimac AB, www.dimac.se
#import "../misc/lib/jmail.dll"     // eMail-Komponente von Dimac AB, www.dimac.se


namespace sos {
namespace spooler {


//-----------------------------------------------------------------------------------------Com_mail

struct Com_mail : spooler_com::Imail, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Mail" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_mail                    ( Spooler* );
                               ~Com_mail                    ();
                                
    void  __stdcall             init                        ();

    USE_SOS_OLE_OBJECT

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

    STDMETHODIMP                add_file                    ( BSTR filename, BSTR content_type, BSTR encoding );
  //STDMETHODIMP                add_attachment              ( BSTR filename, BSTR data );

    STDMETHODIMP            put_smtp                        ( BSTR smtp )                           { _smtp_server = smtp; return NOERROR; }
    STDMETHODIMP            get_smtp                        ( BSTR* smtp )                          { *smtp = _smtp_server; return NOERROR; }

    STDMETHODIMP            put_queue_dir                   ( BSTR );
    STDMETHODIMP            get_queue_dir                   ( BSTR* );

    STDMETHODIMP                add_header_field            ( BSTR field_name, BSTR value );

    STDMETHODIMP                dequeue                     ( int* );

    int __stdcall               send                        ();

  private:
    void                        add_to_recipients           ( BSTR recipients, char recipient_type );
    void                        get_recipients              ( CComBSTR& result_bstr );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    CComBSTR                   _to;
    CComBSTR                   _cc;
    CComBSTR                   _bcc;
    CComBSTR                   _from;
    string                     _subject;
    CComBSTR                   _body;
    _bstr_t                    _smtp_server;

    jmail::IMessagePtr         _msg;
};

} //namespace spooler
} //namespace sos


#endif
#endif
