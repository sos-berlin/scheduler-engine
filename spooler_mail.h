// $Id: spooler_mail.h,v 1.1 2002/03/19 18:56:28 jz Exp $

#ifndef __SPOOLER_MAIL_H
#define __SPOOLER_MAIL_H
#ifdef SYSTEM_WIN

#include <map>
#include "../kram/olestd.h"
#include "../kram/sysxcept.h"
#include "../kram/sosscrpt.h"
#include "../kram/com.h"
#include "../kram/com_server.h"
#include "../kram/sos_mail_jmail.h"


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

    STDMETHODIMP                add_file                    ( BSTR filename, BSTR content_type );                      
  //STDMETHODIMP                add_attachment              ( BSTR filename, BSTR data );

    STDMETHODIMP            put_smtp                        ( BSTR );
    STDMETHODIMP            get_smtp                        ( BSTR* );

    STDMETHODIMP                add_header_field            ( BSTR field_name, BSTR value );

    void __stdcall              send                        ();

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Sos_ptr<Jmail_message>     _msg;

    CComBSTR                   _subject;
    CComBSTR                   _from;
    CComBSTR                   _to;
    CComBSTR                   _cc;
    CComBSTR                   _bcc;
};

} //namespace spooler
} //namespace sos


#endif
#endif
