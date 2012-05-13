// $Id: spooler_xslt_stylesheet.h 13198 2007-12-06 14:13:38Z jz $

#ifndef __SPOOLER_XSLT_STYLESHEET_H
#define __SPOOLER_XSLT_STYLESHEET_H

#include "../zschimmer/xslt_java.h"


namespace sos {
namespace scheduler {

//----------------------------------------------------------------------------------Xslt_stylesheet

struct Xslt_stylesheet : idispatch_implementation< Xslt_stylesheet, spooler_com::Ixslt_stylesheet >, 
                         xml::Xslt_stylesheet,
                         spooler_com::Ihas_java_class_name
{
    static Class_descriptor        class_descriptor;
    static const com::Com_method  _methods[];


                                Xslt_stylesheet             ();
                               ~Xslt_stylesheet             ();

    static HRESULT              Create_instance             ( zschimmer::com::object_server::Session*, ptr<Object>*, const IID&, ptr<IUnknown>* result );  // Für Proxy


    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Xslt_stylesheet"; }


    // interface Ixslt_stylesheet
    STDMETHODIMP                Close                       ();
    STDMETHODIMP                Load_xml                    ( BSTR, spooler_com::Ixslt_stylesheet** );
    STDMETHODIMP                Load_file                   ( BSTR, Ixslt_stylesheet** );
    STDMETHODIMP                Apply_xml                   ( BSTR, BSTR* );

  private:
    Fill_zero                  _zero_;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
