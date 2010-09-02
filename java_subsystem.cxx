// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../kram/sos_java.h"

#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__Scheduler.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__SpoolerC.h"
typedef javaproxy::com::sos::scheduler::kernel::core::Scheduler SchedulerJ;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------Java_objects

struct Java_objects : Object
{
    SchedulerJ                 _schedulerJ;
};

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

    void                        initialize_java_sister      ();
    void                        register_proxy_classes      ();


    // Java_subsystem_interface:
    javabridge::Vm*             java_vm                     ()                                      { return _java_vm; }


  private:
    ptr<javabridge::Vm>        _java_vm;
    ptr<Java_objects>          _java_objects;
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
}

//--------------------------------------------------------------Java_subsystem::~Java_subsystem
    
Java_subsystem::~Java_subsystem()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << " " << x.what() ); }
}

//--------------------------------------------------------------------------Java_subsystem::close
    
void Java_subsystem::close()
{
    if( _java_vm )  _java_vm->set_log( NULL );

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wir Java stehen und schließen es erst am Schluss
    _subsystem_state = subsys_stopped;
}

//-------------------------------------------------------------Java_subsystem::subsystem_initialize

bool Java_subsystem::subsystem_initialize()
{
    _java_vm = get_java_vm( false );
    _java_vm->set_destroy_vm( false );   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal (auch für Dateityp jdbc), wahrscheinlich wegen Hostware ~Sos_static.

    _subsystem_state = subsys_initialized;
    return true;
}

//-------------------------------------------------------------------Java_subsystem::subsystem_load

bool Java_subsystem::subsystem_load()
{
    _java_vm->set_log( _log );
    _java_vm->prepend_class_path( _spooler->_config_java_class_path );
    _java_vm->set_options( _spooler->_config_java_options );

    if( _spooler->_ignore_process_classes ||
        _spooler->scheduler_script_subsystem()->needs_java() )     // Die Java-Jobs laufen mit unserer JVM
    {
        string java_work_dir = _spooler->java_work_dir();
        _java_vm->set_work_dir( java_work_dir );
        _java_vm->prepend_class_path( java_work_dir );
    }

    if( _spooler->_has_java )     // Nur True, wenn Java-Job nicht in separatem Prozess ausgeführt wird.
    {
        Java_module_instance::init_java_vm( _java_vm );
    }

#ifdef Z_DEBUG
    Java_module_instance::init_java_vm( _java_vm );
    initialize_java_sister();
#endif
    
    _subsystem_state = subsys_loaded;
    return true;
}

//-----------------------------------------------------------Java_subsystem::initialize_java_sister

void Java_subsystem::initialize_java_sister()
{
    register_proxy_classes();

    _java_objects = Z_NEW( Java_objects );
    _java_objects->_schedulerJ = SchedulerJ::new_instance(_spooler->j());
    _java_objects->_schedulerJ.test("Hallo, hier ist C++");
    _java_objects->_schedulerJ.test2("Hallo, hier ist nochmal C++");
}

//-----------------------------------------------------------Java_subsystem::register_proxy_classes

void Java_subsystem::register_proxy_classes()
{
    Job             ::register_cpp_proxy_class_in_java();
    Job_subsystem   ::register_cpp_proxy_class_in_java();
    Prefix_log      ::register_cpp_proxy_class_in_java();
    Spooler         ::register_cpp_proxy_class_in_java();
}

//---------------------------------------------------------------Java_subsystem::subsystem_activate

bool Java_subsystem::subsystem_activate()
{
    _subsystem_state = subsys_active;
    return true;
}

//-----------------------------------------Java_subsystem_interface::classname_of_scheduler_object

string Java_subsystem_interface::classname_of_scheduler_object(const string& objectname)
{
    return "sos/spooler/" + replace_regex_ext( objectname, "^(spooler_)?(.*)$", "\\u\\2" );    // "spooler_task" -> "sos.spooler.Task"
}

//-----------------------------------------Java_subsystem_interface::instance_of_scheduler_object

ptr<javabridge::Java_idispatch> Java_subsystem_interface::instance_of_scheduler_object( IDispatch* idispatch, const string& objectname)
{
    string java_class_name = classname_of_scheduler_object(objectname);
    return Z_NEW( javabridge::Java_idispatch( idispatch, true, java_class_name.c_str() ) );
}


//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
