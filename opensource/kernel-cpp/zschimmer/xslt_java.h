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
                               ~Xslt_stylesheet             ();

    void                        release                     ()                                      {}
    void                        load                        ( const Document_ptr& stylesheet );
    void                        load_file                   ( const string& filename );
    Document_ptr                apply                       ( const Document_ptr& );
    Document_ptr                apply                       ( const Document_ptr&, const Xslt_parameters& );

  private:
    CppXsltStylesheetJ         _stylesheetJ;
    string                     _path;
};

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer

#endif
