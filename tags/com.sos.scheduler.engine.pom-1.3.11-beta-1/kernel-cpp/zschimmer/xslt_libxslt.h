// $Id$

#ifndef __ZSCHIMMER_XSLT_LIBXML2_H
#define __ZSCHIMMER_XSLT_LIBXML2_H


#include "xml_libxml2.h"
#include "z_com_server.h"

/*
#ifdef Z_WINDOWS

#   if defined _DEBUG
#       import "debug/xslt_stylesheet.tlb"   rename_namespace("odl") raw_interfaces_only named_guids
#    else
#       import "release/xslt_stylesheet.tlb" rename_namespace("odl") raw_interfaces_only named_guids
#   endif

#endif
*/

namespace zschimmer {
namespace xml {
namespace libxml2 {

struct Xslt_stylesheet;

//----------------------------------------------------------------------------------Xslt_parameters

struct Xslt_parameters : Non_cloneable
{
  //static Xslt_parameters      create                      ( const std::vector<string>& );

                                Xslt_parameters             ( int count = 0 );
                               ~Xslt_parameters             ();

    void                        close                       ();
    void                        allocate                    ( int size );
    void                        set_xpath                   ( int i, const string& name, const string& value );
    void                        set_string                  ( int i, const string& name, const string& value );

  private:
    friend                      struct Xslt_stylesheet;

    int                        _n;
    const char**               _array;
};

//----------------------------------------------------------------------------------Xslt_stylesheet

struct Xslt_stylesheet_ptr : Object, Non_cloneable
{
                                Xslt_stylesheet_ptr         ( _xsltStylesheet* );
                               ~Xslt_stylesheet_ptr         ();

                                operator _xsltStylesheet*   () const                                { return _ptr; }
    bool                        operator !                  () const                                { return _ptr == NULL; }
    Xslt_stylesheet_ptr&        operator =                  ( _xsltStylesheet* );

    void                        assign                      ( _xsltStylesheet* );


  private:
    friend struct               Xslt_stylesheet;

    _xsltStylesheet*           _ptr;
};

//----------------------------------------------------------------------------------Xslt_stylesheet

struct Xslt_stylesheet : //com::idispatch_implementation< Xslt_stylesheet, odl::Ixslt_stylesheet >,
                         Libxml2_error_text
{
                                Xslt_stylesheet             ( Xslt_stylesheet_ptr* = NULL );
                                Xslt_stylesheet             ( const string& xml_or_filename );
                                Xslt_stylesheet             ( const BSTR xml_or_filename );
                               ~Xslt_stylesheet             ();


    Xslt_stylesheet&            operator =                  ( Xslt_stylesheet_ptr* p )              { _stylesheet_ptr = p;  return *this; }

                                operator _xsltStylesheet*   () const                                { return _stylesheet_ptr->_ptr; }

    void                        release                     ();
    bool                        is_xml                      ( const string& );
    bool                        is_xml                      ( const BSTR );
    string                      path                        () const                                { return _path; }
    void                        load                        ( const Document_ptr& stylesheet );
    bool                        loaded                      () const                                { return _stylesheet_ptr != NULL; }
    void                        load_file                   ( const string& filename );
    void                        set_parameter               ( const string& name, const string& value );
 //? Document_ptr                apply                       ( const Document_ptr&, const std::vector<string>& parameters = std::vector<string>() );
    Document_ptr                apply                       ( const Document_ptr& );
    Document_ptr                apply                       ( const Document_ptr&, const Xslt_parameters& );
    void                        write_result_to_file        ( const Document_ptr&, const string& filename );
    string                      xml_from_result             ( const Document_ptr& );


  private:
    void                        prepare_parsing             ();

    ptr<Xslt_stylesheet_ptr>   _stylesheet_ptr;
    string                     _path;
};

//------------------------------------------------------------------------------Com_xslt_stylesheet
/*
struct Com_xslt_stylesheet : com::idispatch_implementation< Xslt_stylesheet, odl::Ixslt_stylesheet >,
                             Xslt_stylesheet
{
    static Class_descriptor     class_descriptor;
    static const com::Com_method  _methods[];


                                Xslt_stylesheet_ptr         ();
                               ~Xslt_stylesheet_ptr         ();


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)_java_class_name.c_str(); }


    // interface Ixslt_stylesheet
    STDMETHODIMP                Load                        ( BSTR );
    STDMETHODIMP                Transform_xml               ( BSTR, BSTR* );
    STDMETHODIMP                Transform_xml_to_file       ( BSTR, BSTR );

  private:
    string                     _java_class_name;
};
*/
//-------------------------------------------------------------------------------------------------

} //namespace libxml2
} //namespace xml
} //namespace zschimmer

#endif
