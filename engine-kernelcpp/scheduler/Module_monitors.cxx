#include "spooler.h"

namespace sos {
namespace scheduler {


const string default_monitor_name = "scheduler";


Module_monitors::Module_monitors(Module* main_module) : 
    _zero_(this +1), 
    _main_module(main_module),
    _job(dynamic_cast<Job*>(_main_module->_file_based))
{}


void Module_monitors::set_dom(const xml::Element_ptr& element) {
    if (_is_loaded) z::throw_xc(Z_FUNCTION);
    if (element.nodeName_is("monitor.use")) {
        Absolute_path path = Absolute_path(element.getAttribute_mandatory("monitor"));
        ptr<Named_monitor_reference> ref = Z_NEW(Named_monitor_reference(path, element.hasAttribute("ordering"), element.int_getAttribute("ordering", 0)));
        _monitor_reference_map[ref->monitor_name()] = +ref; 
        add_requisite(Requisite_path(spooler()->monitor_subsystem(), path));
    }
    else
    if (element.nodeName_is("monitor")) {
        string name = element.getAttribute( "name", default_monitor_name );
        ptr<Module_monitor> module_monitor;
        if (Anonymous_monitor_reference* anon = dynamic_cast<Anonymous_monitor_reference*>(monitor_reference_or_null(name))) {
            module_monitor = anon->module_monitor();
        } else {
            ptr<Module> module = Z_NEW(Module(spooler(), _main_module->_file_based, spooler()->include_path(), &_main_module->_log));
            module_monitor = Z_NEW(Module_monitor(name, module));
            add_module_monitor(module_monitor);  // Possibly overriding existing, equally named Named_monitor_reference
        }
        module_monitor->set_dom(element);
    }
    else {
        assert(false);
        z::throw_xc("SCHEDULER-409", "monitor", element.nodeName());
    }
}


void Module_monitors::add_module_monitor(Module_monitor* monitor) {
    if (_is_loaded) z::throw_xc(Z_FUNCTION);
    ptr<Anonymous_monitor_reference> ref = Z_NEW(Anonymous_monitor_reference(monitor));
    _monitor_reference_map[ref->monitor_name()] = +ref; 
}


void Module_monitors::initialize() {
    if (_is_loaded) z::throw_xc(Z_FUNCTION);
    Z_FOR_EACH(Monitor_reference_map, _monitor_reference_map, i) {
        if (Anonymous_monitor_reference* a = dynamic_cast<Anonymous_monitor_reference*>(+i->second)) {
            a->module_monitor()->initialize();
        }
    }
}


bool Module_monitors::try_load() {
    if (_is_loaded) 
        return true;
    else {
        assert(_module_monitors.empty());
        bool ok = try_load_named_monitors();
        if (!ok) 
            return false;
        else {
            _module_monitors = prepare_module_monitors();
            _is_loaded = true;
            return true;
        }
    }
}

bool Module_monitors::try_load_named_monitors() {
    assert(_module_monitors.empty());
    Z_FOR_EACH_CONST(Monitor_reference_map, _monitor_reference_map, i) {
        Monitor_reference* ref = i->second;
        if (Named_monitor_reference* r = dynamic_cast<Named_monitor_reference*>(ref)) {
            if (Monitor* monitor = spooler()->monitor_subsystem()->monitor_or_null(r->path()))
                r->set_module_monitor(monitor->module_monitor());
            else
                return false;
        } 
    }
    return true;
}


vector<Module_monitor*> Module_monitors::prepare_module_monitors() const {
    vector<Monitor_reference*> refs;
    refs.reserve(_monitor_reference_map.size());
    Z_FOR_EACH_CONST(Monitor_reference_map, _monitor_reference_map, i) refs.push_back(i->second);
    sort(refs.begin(), refs.end(), Monitor_reference::less_ordering);
    
    vector<Module_monitor*> result;
    result.reserve(refs.size());
    Z_FOR_EACH_CONST(vector<Monitor_reference*>, refs, i)  result.push_back((*i)->module_monitor());
    return result;
}


bool Module_monitors::on_requisite_loaded(File_based* requisite) {
    if (Job* job = _job) {
        if (Monitor* monitor = dynamic_cast<Monitor*>(requisite)) {
            if (try_load()) {
                return job->on_monitors_loaded();
            }                 
        }
    }
    return false;
}


bool Module_monitors::on_requisite_to_be_removed(File_based* file_based) {
    _is_loaded = false;
    _module_monitors.clear();
    if (Job* job = _job) {
        if (Monitor* monitor = dynamic_cast<Monitor*>(file_based)) {
            job->on_monitor_to_be_removed(monitor);
        }
    }
    return true;
}


vector<Module_monitor*> Module_monitors::module_monitors() {
    if (!_is_loaded) z::throw_xc(Z_FUNCTION);
    return _module_monitors;
}


Module_monitors::Monitor_reference* Module_monitors::monitor_reference_or_null(const string& name) {
    Monitor_reference_map::iterator i = _monitor_reference_map.find(name);
    return i == _monitor_reference_map.end() ? NULL : i->second;
}


Has_log* Module_monitors::log() { 
    return _main_module->_log.base_log();
}


Spooler* Module_monitors::spooler() const {
    Spooler* result = _main_module->_spooler;
    if (!result) z::throw_xc(Z_FUNCTION);
    return result;
}

}} // namespaces
