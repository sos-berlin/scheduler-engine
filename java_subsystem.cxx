// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../kram/sos_java.h"
#include "javaproxy.h"
#include "jni__register_native_classes.h"

#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__SpoolerC.h"

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------------Java_subsystem

struct Java_subsystem : Java_subsystem_interface
{
                                Java_subsystem              ( Scheduler* );
                               ~Java_subsystem              ();


    // Subsystem:
    void                        close                       ();
    string                      name                        () const                                { return "java"; }
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();
    void                        register_proxy_classes      ();


    // Java_subsystem_interface:
    javabridge::Vm*             java_vm                     ()                                      { return _java_vm; }
    void                    set_java_options                ( const string& x )                     { _java_vm->set_options(x); }
    void                        prepend_class_path          ( const string& x )                     { _java_vm->prepend_class_path(x); }
    SchedulerJ&                 schedulerJ                  ()                                      { return _schedulerJ; }
    xml::Element_ptr            dom_element                 (const xml::Document_ptr&);

  private:
    ptr<javabridge::Vm>        _java_vm;
    SchedulerJ                 _schedulerJ;
  //PlatformJ                  _platformJ;
};

//-------------------------------------------------------------------------------new_java_subsystem

ptr<Java_subsystem_interface> new_java_subsystem( Scheduler* scheduler )
{
    ptr<Java_subsystem> java_subsystem = Z_NEW( Java_subsystem( scheduler ) );
    return +java_subsystem;
}

//-------------------------------------------------------------------Java_subsystem::Java_subsystem

Java_subsystem::Java_subsystem( Scheduler* scheduler )
: 
    Java_subsystem_interface( scheduler, type_java_subsystem )
{
    _java_vm = get_java_vm( false );
    _java_vm->set_destroy_vm( false );   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal (auch für Dateityp jdbc), wahrscheinlich wegen Hostware ~Sos_static.
    _java_vm->set_log( _log );
    _java_vm->prepend_class_path( subst_env( read_profile_string( _spooler->_factory_ini, "java", "class_path" ) ) );

    if( _spooler->_ignore_process_classes )//||
        //_spooler->scheduler_script_subsystem()->needs_java() )     // Die Java-Jobs laufen mit unserer JVM
    {
        string java_work_dir = _spooler->java_work_dir();
        _java_vm->set_work_dir( java_work_dir );
        _java_vm->prepend_class_path( java_work_dir );
    }
}

//------------------------------------------------------------------Java_subsystem::~Java_subsystem
    
Java_subsystem::~Java_subsystem()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << " " << x.what() ); }
}

//----------------------------------------------------------------------------Java_subsystem::close
    
void Java_subsystem::close()
{
    if (_schedulerJ) {
        _schedulerJ.onClose();
        _schedulerJ.assign_(NULL);
    }

    if( _java_vm )  _java_vm->set_log( NULL );

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wir Java stehen und schließen es erst am Schluss
    _subsystem_state = subsys_stopped;
}

//-------------------------------------------------------------Java_subsystem::subsystem_initialize

bool Java_subsystem::subsystem_initialize()
{
    Java_module_instance::init_java_vm( _java_vm );
    register_native_classes();
    _schedulerJ.assign_(SchedulerJ::new_instance(_spooler->j(), _spooler->java_main_context()));
    _subsystem_state = subsys_initialized;
    return true;
}

//-------------------------------------------------------------------Java_subsystem::subsystem_load

bool Java_subsystem::subsystem_load()
{
    _schedulerJ.onLoad();
    _subsystem_state = subsys_loaded;
    return true;
}

//---------------------------------------------------------------Java_subsystem::subsystem_activate

bool Java_subsystem::subsystem_activate()
{
    _schedulerJ.onActivate();
    _subsystem_state = subsys_active;
    return true;
}

//------------------------------------------Java_subsystem_interface::classname_of_scheduler_object

string Java_subsystem_interface::classname_of_scheduler_object(const string& objectname)
{
    return "sos/spooler/" + replace_regex_ext( objectname, "^(spooler_)?(.*)$", "\\u\\2" );    // "spooler_task" -> "sos.spooler.Task"
}

//-------------------------------------------Java_subsystem_interface::instance_of_scheduler_object

ptr<javabridge::Java_idispatch> Java_subsystem_interface::instance_of_scheduler_object( IDispatch* idispatch, const string& objectname)
{
    string java_class_name = classname_of_scheduler_object(objectname);
    return Z_NEW( javabridge::Java_idispatch( idispatch, true, java_class_name.c_str() ) );
}

//----------------------------------------------------------------------Java_subsystem::dom_element

xml::Element_ptr Java_subsystem::dom_element(const xml::Document_ptr& dom)
{
    xml::Element_ptr result = dom.createElement("java_subsystem");
    result.append_new_comment("For debugging only");
    javabridge::Vm* vm = javabridge::Vm::get_vm(false);
    const javabridge::Statistics& stat = vm->_statistics;

    result.setAttribute("GlobalRef", stat._NewGlobalRef_count - stat._DeleteGlobalRef_count );
    result.setAttribute("NewGlobalRef", stat._NewGlobalRef_count );
    result.setAttribute("DeleteGlobalRef", stat._DeleteGlobalRef_count );

    if (javabridge::Jobject_debug_register* reg = javabridge::Vm::static_vm->_jobject_debug_register) {
        xml::Element_ptr objects_element = result.append_new_element("objects");
        typedef javabridge::Jobject_debug_register::Class_object_counter_map Map;
        Map map = reg->class_object_counter_map();
        Z_FOR_EACH_CONST(Map, map, it) {
            xml::Element_ptr e = objects_element.append_new_element("object");
            e.setAttribute("class", it->first);
            e.setAttribute("count", it->second);
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
