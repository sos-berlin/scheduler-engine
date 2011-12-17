/**
 * \file spooler_module_script
 * \brief script-Verarbeitung über JAVA interface
 * \details 
 * Verarbeitet Jobs, die in einer Scriptsprache geschrieben sind über ein JAVA-Modul
 *
 * \author SS
 * \version 2.1.1 - 2010-05-07
 * \copyright © 2010 by SS
 * <div class="sos_branding">
 *    <p>© 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
#include "spooler.h"
#include "../javaproxy/java__lang__String.h"
#include "../zschimmer/java_com.h"

using namespace zschimmer::javabridge;

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------Script_module::Script_module
    
Script_module::Script_module( Spooler* spooler, Prefix_log* log )
:
    Module( spooler, (File_based*)NULL, "::NO-INCLUDE-PATH::", log )
{
    _kind = kind_scripting_engine_java;
    _set = true;
}

//---------------------------------------------------Internal_instance_base::Script_module_instance
    
Script_module_instance::Script_module_instance( Module* module )             
: 
    Module_instance(module),
    _zero_(this+1),
    _java_module_instance( javaproxy::com::sos::scheduler::engine::kernel::scripting::APIModuleInstance::new_instance( module->_language, module->read_source_script()) )
{
}

//----------------------------------------------------------------Script_module_instance::add_obj
    
void Script_module_instance::add_obj( IDispatch* idispatch, const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::add_obj name=" << name << "\n");

    ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(idispatch, name);
    _added_jobjects.push_back( java_idispatch );
    _java_module_instance.addObject( java_idispatch->get_jobject(), name );            // registriert das Object in ScriptInterface
    Module_instance::add_obj( idispatch, name );
}

//-----------------------------------------------------------Script_module_instance::call

Variant Script_module_instance::call( const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::call name=" << name << "\n");
    Com_env env = _module->_java_vm->jni_env();
    javaproxy::java::lang::Object result = _java_module_instance.call(name);
    return (result == NULL) ? empty_variant : env.jobject_to_variant( result ); 
}

//-------------------------------------------------------------------Script_module_instance::call

Variant Script_module_instance::call( const string& name, const Variant& p1, const Variant& p2 )
{
    if (p2 != missing_variant ) 
       assert(0), z::throw_xc( "SCHEDULER-481", name, "(p1,p2)" );

    Z_LOG2("scheduler","Script_module_instance::call\n");
    Com_env env = _module->_java_vm->jni_env();
    javaproxy::java::lang::Object result = _java_module_instance.call(name,p1.as_bool());
    return (result == NULL) ? empty_variant : env.jobject_to_variant( result ); 
}

//------------------------------------------------------------Script_module_instance::name_exists

bool Script_module_instance::name_exists( const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::name_exists name=" << name << "\n");
    return (_java_module_instance.nameExists(name) != 0);  // weil name_exists jboolean zurück gibt (will JZ noch ändern)
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

