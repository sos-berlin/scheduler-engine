#include "spooler.h"

namespace sos {
namespace scheduler {


Monitor::Monitor(Spooler* spooler) : 
    file_based<Monitor, Monitor_folder, Monitor_subsystem>(spooler->monitor_subsystem(), this, Scheduler_object::type_monitor),
    javabridge::has_proxy<Monitor>(spooler),
    _zero_(this+1),
    _module_monitor(Z_NEW(Module_monitor(Z_NEW(Module(spooler, this, spooler->include_path(), log(), true)))))
{
}


bool Monitor::on_initialize() {
    _module_monitor->_monitor_name = path();
    _module_monitor->initialize();
    return true;
}


bool Monitor::on_load() {
    return true;
}


bool Monitor::on_activate() {
    return true;
}


bool Monitor::can_be_removed_now() {
    return true;
}


void Monitor::set_dom(const xml::Element_ptr& monitor_element) {
    _module_monitor->set_dom(monitor_element);
}


xml::Element_ptr Monitor::dom_element(const xml::Document_ptr& dom_document, const Show_what& show_what) {
    xml::Element_ptr result = dom_document.createElement("monitor");
    fill_file_based_dom_element( result, show_what );
    result.setAttribute("ordering", _module_monitor->ordering());
    return result;
}


}}
