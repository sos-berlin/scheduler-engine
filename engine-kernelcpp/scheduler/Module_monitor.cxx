#include "spooler.h"

namespace sos {
namespace scheduler {

const string default_monitor_name = "scheduler";
const int default_ordering = 1;


Module_monitor::Module_monitor() : 
    _zero_(this + 1), 
    _ordering(default_ordering),
    _monitor_name(default_monitor_name)
{}

Module_monitor::Module_monitor(Module* module) :
    _zero_(this + 1), 
    _ordering(default_ordering),
    _monitor_name(default_monitor_name),
    _module(module)
{}

Module_monitor::Module_monitor(const string& name, Module* module) :
    _zero_(this + 1), 
    _ordering(default_ordering),
    _monitor_name(name),
    _module(module)
{}

void Module_monitor::initialize() {
    _module->init();
}


void Module_monitor::set_dom(const xml::Element_ptr& monitor_element) {
    if (!monitor_element.nodeName_is("monitor"))  assert(0), z::throw_xc("SCHEDULER-409", "monitor", monitor_element.nodeName());
    
    string name = monitor_element.getAttribute("name", default_monitor_name);
    _ordering = monitor_element.int_getAttribute("ordering", _ordering);

    DOM_FOR_EACH_ELEMENT(monitor_element, e) {
        if (e.nodeName_is("script")) {
            _module->set_dom(e);
        }
    }
}

ptr<Module_instance> Module_monitor::create_module_instance(Task* task_or_null) const {
    return _module->create_instance((Process_class*)NULL, "", task_or_null);
}


}} // namespace
