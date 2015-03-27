#include "spooler.h"

namespace sos {
namespace scheduler {

//-------------------------------------------------Module_monitor_instance::Module_monitor_instance

Module_monitor_instance::Module_monitor_instance( Module_monitor* monitor, Module_instance* module_instance )
:
    _module_instance(module_instance),
    _obj_name( "Script_monitor " + monitor->monitor_name() )
{
}

}} // namespaces
