// $Id: spooler_subprocess.h 3668 2005-05-17 10:47:25Z jz $

#ifndef __SPOOLER_XSLT_STYLESHEET_H
#define __SPOOLER_XSLT_STYLESHEET_H

#include "../zschimmer/xslt_libxslt.h"


namespace sos {
namespace spooler {



//----------------------------------------------------------------------------------Xslt_stylesheet

struct Xslt_stylesheet : idispatch_implementation< Xslt_stylesheet, spooler_com::Ixslt_stylesheet >, 
                         xml::Xslt_stylesheet,
                         spooler_com::Ihas_java_class_name
{
    static Class_descriptor        class_descriptor;
    static const com::Com_method  _methods[];


                                Xslt_stylesheet             ();
                              //Xslt_stylesheet             ( const string& xml_or_filename );
                              //Xslt_stylesheet             ( const BSTR xml_or_filename );
                               ~Xslt_stylesheet             ();


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
  //STDMETHODIMP                Transform_xml_to_file       ( BSTR, BSTR );


  private:
    Fill_zero                  _zero_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
