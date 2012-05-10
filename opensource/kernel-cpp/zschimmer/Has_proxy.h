// $Id$

#ifndef __ZSCHIMMER_HAS_JAVA_PROXY_
#define __ZSCHIMMER_HAS_JAVA_PROXY_

#include <typeinfo>
#include "java.h"
#include "Proxy_class.h"

namespace zschimmer {
namespace javabridge {

struct Proxy_class;

//----------------------------------------------------------------------------------------Has_proxy
// Legt bei Bedarf einen CppProxy an (vom Java-C++-Generator erzeugt) und und löst ihn am Ende wieder.
// Der CppProxy hält nicht dieses C++-Object. Wenn das C++-Objekt zerstört wird, wird die Verbindung vom
// CppProxy zum C++-Objekt gelöst und die nächste Verwendung des CppProxy liefert den Fehler Z-JAVA-111.

struct Has_proxy
{
                                Has_proxy                   (Has_proxy* sister_context_proxy);
    virtual                    ~Has_proxy                   ();

    jobject                     j                           ()                                      { return java_proxy_jobject(); }
    jobject                     java_proxy_jobject          ();
    virtual jobject             java_sister                 (); 

    static jobject              jobject_of                  (Has_proxy* a)                          { return a? a->java_proxy_jobject() : NULL; }

protected:
    virtual const Proxy_class*  proxy_class                 () const                                = 0;

private:
    template<class SELF> friend struct has_proxy;

    jlong                       cpp_reference               ()                                      { return (jlong)this; }
    static Has_proxy*           of_cpp_reference_           (jlong, const char* debug_string);

    void                        cache_proxy_class           ();
    void                    set_reference_in_proxy          (jlong);
    void                        create_proxy                ();

    Proxy_class const*         _proxy_class;
    Global_jobject2            _proxy;
    Has_proxy* const           _sister_context_proxy;       // Kontext für die Java-Schwester oder NULL
    Global_jobject2            _java_sister;                // Manche C++-Klassen haben neben dem C++-Proxy eine Java-Schwester, eine von C++ unabhängige Klasse
};

//--------------------------------------------------------------------------------------has_proxy<>

template<class SELF>
struct has_proxy : Has_proxy
{
                                has_proxy                   (Has_proxy* sister_context_proxy = NULL) : Has_proxy(sister_context_proxy) {}
    static SELF*                of_cpp_reference            (jlong cpp_reference, const char* debug_string) { return (SELF*)(has_proxy<SELF>*)of_cpp_reference_(cpp_reference, debug_string); }
    static void                 register_cpp_proxy_class_in_java();

    static const class_factory<Proxy_class> proxy_class_factory;

  protected:
    const Proxy_class*          proxy_class                 () const                                { return (const Proxy_class*)proxy_class_factory.clas(); }
};

//----------------------------------------------------------------------------------proxy_jobject<>

struct Proxy_jobject {
  protected:
                                Proxy_jobject               (jobject jo = NULL)                     : _jobject(jo) {}

  public:
    jobject                     get_jobject                 () const                                { return _jobject; }
    void                        set_jobject                 (jobject jo)                            { _jobject = jo; }

  private:
    jobject                    _jobject;
};

//----------------------------------------------------------------------------------proxy_jobject<>
// Typsicheres jobject, für Parameterübergabe von C++ an Java, 
// damit nicht Global_jobject2 mit NewGlobalRef() und DeleteGlobalRef() verwendet werden muss.

template<class PROXY>   // PROXY: Vom C++/Java-Generator erzeugte Klasse für Java-Proxy, z.B. SpoolerC
struct proxy_jobject : Proxy_jobject
{
                                proxy_jobject               (jobject jo = NULL)                     : Proxy_jobject(jo) { if (jo) PROXY::java_class_()->assert_is_assignable_from(jo); }
};

//--------------------------------------------------------------------------------java_array_from_c

jobjectArray java_array_from_c(const std::vector<string>&);
jbyteArray java_byte_array_from_c(const string&);
string string_from_java_byte_array(jbyteArray);

//Für Has_proxy nicht getestet:
//template<class T>
//jobjectArray java_array_from_c(const std::vector<T>& v) {
//    JNIEnv* jenv = jni_env();
//    Local_frame java_frame (10);
//    jobjectArray result;
//    result = jenv->NewObjectArray(v.size(), Vm::static_vm->standard_classes()->_java_lang_object_class, NULL);
//    if (result == NULL) return NULL;
//    for (int i = 0; i < v.size(); i++)
//        jenv->SetObjectArrayElement(result, i, v[i]->java_proxy_jobject());
//    return result;
//}

} //namespace javabridge
} //namespace zschimmer

#endif
