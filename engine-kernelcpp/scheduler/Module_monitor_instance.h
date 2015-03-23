#ifndef __MODULE_MONITOR_INSTANCE_H
#define __MODULE_MONITOR_INSTANCE_H

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------Module_monitor_instance

struct Module_monitor_instance : Object
{
                                Module_monitor_instance     ( Module_monitor*, Module_instance* );

    string                      obj_name                    () const                                { return _obj_name; }

    ptr<Module_instance>       _module_instance;
    string                     _obj_name;
};


}} // namespaces

#endif
