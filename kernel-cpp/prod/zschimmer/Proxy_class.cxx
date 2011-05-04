// $Id$

#include "java.h"
#include "Proxy_class.h"

namespace zschimmer {
namespace javabridge {

//-------------------------------------------------------------------------Proxy_class::Proxy_class

Proxy_class::Proxy_class(const string& class_name) 
:
    Class(class_name),
    _constructor(this, Parameter_list_signature::of_types(t_void, "com.sos.scheduler.engine.cplusplus.runtime.Sister")),
    _cppReference_field(this, "cppReference", Simple_signature::of_type(t_long)),
    _invalidateCppReference_method(this, "invalidateCppReference", Parameter_list_signature::of_types(t_void)),
    _getSister_method(this, "getSister", Parameter_list_signature::of_types("com.sos.scheduler.engine.cplusplus.runtime.Sister"))
{
}

//------------------------------------------------------------------------Proxy_class::~Proxy_class
    
Proxy_class::~Proxy_class() 
{
}

//------------------------------------------------------------------------Proxy_class::new_instance
    
jobject Proxy_class::new_instance(jobject context_CppProxy) const
{ 
    jobject result = alloc_object();
    Parameters p (context_CppProxy);
    _constructor.call(result, p); 
    return result;
}

//---------------------------------------------------------Java_proxy:class::set_reference_in_proxy

void Proxy_class::set_reference_in_proxy(jobject jo, jlong value) const
{
    if (value == 0) 
        _invalidateCppReference_method.call(jo);
    else
        _cppReference_field.set_long( jo, value );
}

//---------------------------------------------------------------------------Proxy_class::getSister

Global_jobject2 Proxy_class::getSister(jobject jo) const
{
    Global_jobject2 result;
    result.steal_local_ref( _getSister_method.jobject_call(jo) );
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer

