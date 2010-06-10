#ifndef __SCRIPT_MODULE_SCRIPT_INTERFACE_H
#define __SCRIPT_MODULE_SCRIPT_INTERFACE_H

#include "../zschimmer/java.h"
#include "../zschimmer/java_com.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

using namespace std;
using namespace zschimmer;
using namespace zschimmer::javabridge;

namespace sos {
namespace scheduler {
namespace scheduler_java {
//-------------------------------------------------------------------------------------------------
// JAVA-Schnittstelle zur Script-Implementierung
struct ScriptInterface_class : Object_class
{
    ScriptInterface_class( const string& class_name ) : 
        Object_class( class_name ),
        _constructor        ( this, "<init>",         Signature::of_types( t_void, "java.lang.String",  "java.lang.String" ) ),
        _addObject_method   ( this, "addObject",    Signature::of_types( t_void, "java.lang.Object", "java.lang.String" ) ),
        _callBoolean_method ( this, "callBoolean" , Signature::of_types( t_boolean, "java.lang.String" ) ),
        _nameExists_method  ( this, "nameExists" ,  Signature::of_types( t_boolean, "java.lang.String" ) )
    {}

    const Constructor _constructor;
    const Method      _addObject_method;
    const Method      _callBoolean_method;
    const Method      _nameExists_method;

    static const class_factory<ScriptInterface_class> class_factory;     // Für class_factory_based<>
};

//---------------------------------------------------------------------------------------ScriptInterface

struct ScriptInterface : object_impl<ScriptInterface_class>
{
                                     ScriptInterface            ( const string& lang, const string& code ) { clas()->_constructor.construct( *this, Parameters(lang,code) ); }
    void                             add_obj                    ( jobject object, const string& name )     { clas()->_addObject_method.call( *this, Parameters(object,name) ); }
    bool                             call_boolean               ( const string& name )                     { return clas()->_callBoolean_method.bool_call( *this, Parameters(name) ); }
    bool                             name_exists                ( const string& name )                     { return clas()->_nameExists_method.bool_call( *this, Parameters(name) ); }

};

//---------------------------------------------------------------------------------ScriptConnector_class

} //namespace javabridge
} //namespace scheduler
} //namespace sos
#endif
