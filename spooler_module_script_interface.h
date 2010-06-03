#ifndef __SCRIPT_MODULE_SCRIPT_INTERFACE_H
#define __SCRIPT_MODULE_SCRIPT_INTERFACE_H

#include "../zschimmer/java.h"
#include "../zschimmer/java_com.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

using namespace std;
using namespace zschimmer;
using namespace zschimmer::java;

namespace sos {
namespace scheduler {
namespace scheduler_java {
//-------------------------------------------------------------------------------------------------
// JAVA-Schnittstelle zur Script-Implementierung
struct ScriptInterface_class : Object_class
{
    ScriptInterface_class( const string& class_name ) : 
        Object_class( class_name ),
        _constructor        ( this, "<init>"   ,  Signature::of_types( t_void ) ),
        _init_method        ( this, "init",       Signature::of_types( t_void, "java.lang.String" ) ),
        _addObject_method   ( this, "addObject",  Signature::of_types( t_void, "java.lang.Object", "java.lang.String" ) ),
        _call_method        ( this, "call" ,      Signature::of_types( t_boolean, "java.lang.String" ) )
    {}

    const Constructor _constructor;
    const Method      _addObject_method;
    const Method      _init_method;
    const Method      _call_method;

    static const class_factory<ScriptInterface_class> class_factory;     // Für class_factory_based<>
};

//---------------------------------------------------------------------------------------ScriptInterface

struct ScriptInterface : object_impl<ScriptInterface_class>
{
                                ScriptInterface                 ( const string& servicename )            { clas()->_constructor.construct( *this, Parameters() ); }
    void                             add_obj                    ( jobject object, const string& name )   { clas()->_addObject_method.call( *this, Parameters(object,name) ); }
    void                             init                       ( const string& sourcecode )             { clas()->_init_method.call( *this, Parameters(sourcecode) ); }
    bool                             call                       ( const string& name )                   { return clas()->_call_method.bool_call( *this, Parameters(name) ); }
    bool                             name_exists                ( const string& name )  { return true; }

};

//---------------------------------------------------------------------------------ScriptConnector_class

// JAVA-Schnittstelle zum Service-Provider für das ScriptInterface
/*
struct ScriptConnector_class : Object_class
{
    ScriptConnector_class( const string& class_name ) : 
        Object_class( class_name ),
        _constructor           ( this, "<init>",          Signature::of_types( t_void, "java.lang.String" ) ),
        _getService_method     ( this, "getService",      Signature::of_types( "sos.service.scheduler.ModuleScriptInterface" ) ),
        _getServicename_method ( this, "getServicename" , Signature::of_types( "java.lang.String" ) )
    {}

    const Constructor _constructor;
    const Method      _getService_method;
    const Method      _getServicename_method;

    static const class_factory<ScriptConnector_class> class_factory;     // Für class_factory_based<>
};
*/

//-------------------------------------------------------------------------------------ScriptConnector

/*
struct ScriptConnector : object_impl<ScriptConnector_class>
{
                                ScriptConnector     ( const string& servicename )       { clas()->_constructor.construct( *this, Parameters(servicename) ); }

    // ScriptInterface             getService          ()                                  { return clas()->_getService_method.object_call( *this ); }
    string                      getServicename      ()                                  { return clas()->_getServicename_method.string_call( *this ); }

};
*/

//-------------------------------------------------------------------------------------------------

} //namespace java
} //namespace scheduler
} //namespace sos
#endif
