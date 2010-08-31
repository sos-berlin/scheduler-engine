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
    _java_module( javaproxy::com::sos::scheduler::kernel::core::scripting::Module::new_instance( module->_language, module->read_source_script()) )
{
}

//----------------------------------------------------------------Script_module_instance::add_obj
    
void Script_module_instance::add_obj( IDispatch* idispatch, const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::add_obj name=" << name << "\n");

    ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(idispatch, name);
    _added_jobjects.push_back( java_idispatch );
    _java_module.addObject( java_idispatch->get_jobject(), name );            // registriert das Object in ScriptInterface
}

//-----------------------------------------------------------Script_module_instance::call

Variant Script_module_instance::call( const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::call name=" << name << "\n");
    return _java_module.callBoolean(name);
}

//-------------------------------------------------------------------Script_module_instance::call

Variant Script_module_instance::call( const string&, const Variant&, const Variant& )
{
    Z_LOG2("scheduler","Script_module_instance::call\n");
    assert(0), z::throw_xc( Z_FUNCTION );
}

//------------------------------------------------------------Script_module_instance::name_exists

bool Script_module_instance::name_exists( const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::name_exists name=" << name << "\n");
    return (_java_module.nameExists(name) != 0);  // weil name_exists jboolean zurück gibt (will JZ noch ändern)
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

