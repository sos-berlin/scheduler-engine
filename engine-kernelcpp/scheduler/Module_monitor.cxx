#include "spooler.h"

namespace sos {
namespace scheduler {

const string default_monitor_name = "scheduler";


Module_monitor::Module_monitor() : 
    _zero_(this + 1), 
    _ordering(1),
    _name(default_monitor_name)
{}

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

}} // namespace
