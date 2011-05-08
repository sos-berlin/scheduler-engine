// $Id$

#ifndef __ZSCHIMMER_JAVA_PROXY_CLASS_
#define __ZSCHIMMER_JAVA_PROXY_CLASS_

#include "java.h"

namespace zschimmer {
namespace javabridge {

struct Java_proxy_class_inlay;
struct Has_proxy;

//---------------------------------------------------------------------------------Proxy_class

struct Proxy_class : Class
{
                                Proxy_class                 (const string& java_proxy_class_name);
                               ~Proxy_class                 ();

    jobject                     new_instance                (jobject context_CppProxy) const;
    Global_jobject2             getSister                   (jobject jo) const;

 private:
    friend struct Has_proxy;
    void                    set_reference_in_proxy          (jobject, jlong) const;

    Constructor const          _constructor;
    Field const                _cppReference_field;
    Method const               _invalidateCppReference_method;
    Method const               _getSister_method;
    string const               _java_class_name;
};

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer

#endif
