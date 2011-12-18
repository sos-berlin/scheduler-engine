// $Id$

#include "java.h"
#include "Has_proxy.h"
#include "Proxy_class.h"

namespace zschimmer {
namespace javabridge {

//-----------------------------------------------------------------------------Has_proxy::Has_proxy
    
Has_proxy::Has_proxy(Has_proxy* sister_context_proxy)
:
    _proxy_class(NULL),
    _sister_context_proxy(sister_context_proxy)
{
}

//----------------------------------------------------------------------------Has_proxy::~Has_proxy
    
Has_proxy::~Has_proxy() 
{
    if (_proxy)
        set_reference_in_proxy(0);
}

//---------------------------------------------------------------------Has_proxy::cache_proxy_class

void Has_proxy::cache_proxy_class()
{
    if (!_proxy_class)
        _proxy_class = proxy_class();
}

//-------------------------------------------------------------------------Has_proxy::proxy_jobject

jobject Has_proxy::java_proxy_jobject() 
{
    jobject result = _proxy.get_jobject();
    if( !result ) {
        create_proxy();
        result = _proxy.get_jobject();
    }
    return result; 
}

//--------------------------------------------------------------------------Has_proxy::create_proxy

void Has_proxy::create_proxy() 
{
    cache_proxy_class(); 
    jobject context = _sister_context_proxy? _sister_context_proxy->java_sister() : NULL;
    _proxy.steal_local_ref( _proxy_class->new_instance(context) );
    set_reference_in_proxy( cpp_reference() );
}

//----------------------------------------------------------------Has_proxy::set_reference_in_proxy

void Has_proxy::set_reference_in_proxy(jlong value)
{
    _proxy_class->set_reference_in_proxy(java_proxy_jobject(), value);
}

//---------------------------------------------------------------------------Has_proxy::java_sister

jobject Has_proxy::java_sister() 
{
    if (!_java_sister) {
        cache_proxy_class();
        _java_sister.assign_( _proxy_class->getSister(java_proxy_jobject()) );
    }

    return _java_sister;
}

//---------------------------------------------------------------------Has_proxy::of_cpp_reference_

Has_proxy* Has_proxy::of_cpp_reference_(jlong cpp_reference, const char* debug_string)
{ 
    if (!cpp_reference)
        throw_xc("Z-JAVA-111", debug_string);

    return (Has_proxy*)cpp_reference;
}

//--------------------------------------------------------------------------------java_array_from_c

jobjectArray java_array_from_c(const std::vector<string>& v) {
    Env jenv;
    Local_frame java_frame (10);
    jobjectArray result = jenv->NewObjectArray(v.size(), Vm::static_vm->standard_classes()->_java_lang_string_class, NULL);
    if (result != NULL) {  // Keine Exception
        for (int i = 0; i < v.size(); i++)
            jenv->SetObjectArrayElement(result, i, jenv.jstring_from_string(v[i]));
    }
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer
