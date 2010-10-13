// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../kram/sos_java.h"
#include "javaproxy.h"
#include "jni__register_native_classes.h"

#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__SpoolerC.h"

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------Java_objects
// Von Java_subsystem_impl entkoppelt, weil Konstruktur der JavaProxys Java voraussetzt. 
// Also legen wir Java_objects erst an, wenn Java läuft.

struct Java_objects : Object
{
    SchedulerJ                 _schedulerJ;
  //PlatformJ                  _platformJ;
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
    void                        register_proxy_classes      ();


    // Java_subsystem_interface:
    javabridge::Vm*             java_vm                     ()                                      { return _java_vm; }
    void                    set_java_options                ( const string& x )                     { _java_vm->set_options(x); }
    void                        prepend_class_path          ( const string& x )                     { _java_vm->prepend_class_path(x); }
    const SchedulerJ&           schedulerJ                  () const                                { return _java_objects->_schedulerJ; }
  //const PlatformJ&            platformJ                   () const                                { return _java_objects->_platformJ; }


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
    if( _java_objects )  _java_objects->_schedulerJ.close();
    if( _java_vm )  _java_vm->set_log( NULL );

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wir Java stehen und schließen es erst am Schluss
    _subsystem_state = subsys_stopped;
}

//-------------------------------------------------------------Java_subsystem::subsystem_initialize

bool Java_subsystem::subsystem_initialize()
{
#ifndef SUPPRESS_JAVAPROXY
    Java_module_instance::init_java_vm( _java_vm );
    register_native_classes();

    _java_objects = Z_NEW( Java_objects );
    _java_objects->_schedulerJ.assign( SchedulerJ::new_instance(_spooler->j()) );
#endif
    _subsystem_state = subsys_initialized;
    return true;
}

//-------------------------------------------------------------------Java_subsystem::subsystem_load

bool Java_subsystem::subsystem_load()
{
#ifndef SUPPRESS_JAVAPROXY
    _java_objects->_schedulerJ.load();
#endif
    _subsystem_state = subsys_loaded;
    return true;
}

//---------------------------------------------------------------Java_subsystem::subsystem_activate

bool Java_subsystem::subsystem_activate()
{
#ifndef SUPPRESS_JAVAPROXY
    _java_objects->_schedulerJ.activate("Hallo, hier ist C++");
#ifdef Z_DEBUG
    _java_objects->_schedulerJ.activateMonitor();
#endif
#endif

    _subsystem_state = subsys_active;
    return true;
}

//------------------------------------------Java_subsystem_interface::classname_of_scheduler_object

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
