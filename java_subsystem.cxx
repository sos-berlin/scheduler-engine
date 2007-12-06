// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../kram/sos_java.h"

namespace sos {
namespace scheduler {

//---------------------------------------------------------------------------------Java_subsystem

struct Java_subsystem : Java_subsystem_interface
{
                                Java_subsystem              ( Scheduler* );
                               ~Java_subsystem              ();


    // Subsystem:
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();


    // Java_subsystem_interface:
    java::Vm*                   java_vm                     ()                                      { return _java_vm; }


  private:
    ptr<java::Vm>              _java_vm;
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
}

//--------------------------------------------------------------------------Java_subsystem::close
    
void Java_subsystem::close()
{
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

    //if( _spooler->_has_java_source )
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

    _subsystem_state = subsys_loaded;
    return true;
}

//---------------------------------------------------------------Java_subsystem::subsystem_activate

bool Java_subsystem::subsystem_activate()
{
    _subsystem_state = subsys_active;
    return true;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
