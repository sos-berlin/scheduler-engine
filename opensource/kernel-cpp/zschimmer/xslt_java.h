#ifndef __ZSCHIMMER_XSLT_LIBXML2_H
#define __ZSCHIMMER_XSLT_LIBXML2_H

#include "xml_java.h"
#include "z_com_server.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__xml__CppXsltStylesheet.h"
#include "../javaproxy/java__util__HashMap.h"

namespace zschimmer {
namespace xml {

struct Xslt_stylesheet;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::xml::CppXsltStylesheet CppXsltStylesheetJ;
typedef ::javaproxy::java::util::HashMap HashMapJ;

//----------------------------------------------------------------------------------Xslt_parameters

struct Xslt_parameters : Non_cloneable
{
                                Xslt_parameters             ( int count = 0 );
                               ~Xslt_parameters             ();

    void                        allocate                    ( int size );
    void                        set_xpath                   ( int i, const string& name, const string& value );
    void                        set_string                  ( int i, const string& name, const string& value );

  private:
    friend                      struct Xslt_stylesheet;

    struct Parameter {
        string _name;
        string _value;
    };

    std::vector<Parameter>      _parameters;
    HashMapJ                    _hashMapJ;
};

//----------------------------------------------------------------------------------Xslt_stylesheet

struct Xslt_stylesheet {
                                Xslt_stylesheet             ();
                                Xslt_stylesheet             ( const string& xml_or_filename );
                                Xslt_stylesheet             ( const BSTR xml_or_filename );
                               ~Xslt_stylesheet             ();

    void                        release                     ()                                      {}
    bool                        is_xml                      ( const string& );
    bool                        is_xml                      ( const BSTR );
    string                      path                        () const                                { return _path; }
    void                        load                        ( const Document_ptr& stylesheet );
    bool                        loaded                      () const                                { return _stylesheetJ != NULL; }
    void                        load_file                   ( const string& filename );
    void                        set_parameter               ( const string& name, const string& value );
    Document_ptr                apply                       ( const Document_ptr& );
    Document_ptr                apply                       ( const Document_ptr&, const Xslt_parameters& );
    string                      xml_from_result             ( const Document_ptr& );

  private:
    CppXsltStylesheetJ         _stylesheetJ;
    string                     _path;
};

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer

#endif
