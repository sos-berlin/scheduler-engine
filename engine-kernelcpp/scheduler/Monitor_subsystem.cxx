#include "spooler.h"

namespace sos {
namespace scheduler {

Monitor_subsystem::Monitor_subsystem(Spooler* spooler) :
    file_based_subsystem<Monitor>(spooler, this, type_monitor_subsystem),
    javabridge::has_proxy<Monitor_subsystem>(spooler)
{}
    

struct Standard_monitor_subsystem : Monitor_subsystem
{
    public: Standard_monitor_subsystem(Spooler* spooler) :
        Monitor_subsystem(spooler)
    {}

    void close() {
        set_subsystem_state(subsys_stopped);
        file_based_subsystem<Monitor>::close();
    }

    bool subsystem_initialize() {
        file_based_subsystem<Monitor>::subsystem_initialize();
        set_subsystem_state(subsys_initialized);
        return true;
    }

    bool subsystem_load() {
        file_based_subsystem<Monitor>::subsystem_load();
        set_subsystem_state(subsys_loaded);
        return true;
    }

    bool subsystem_activate() {
        set_subsystem_state(subsys_active);
        file_based_subsystem<Monitor>::subsystem_activate();
        return true;
    }

    virtual ptr<Monitor_folder> new_monitor_folder(Folder* folder) {
        return Z_NEW(Monitor_folder(folder));
    }

    string object_type_name() const {
        return "Monitor";
    }

    string filename_extension(void) const {
        return ".monitor.xml";
    }

    string xml_element_name() const {
        return "monitor";
    }

    string xml_elements_name() const {
        return "monitors";
    }

    ptr<Monitor> new_file_based(const std::string&) {
        return Z_NEW(Monitor(spooler()));
    }

    Monitor* monitor(const Absolute_path& job_path) { 
        return file_based(job_path); 
    }

    Monitor* monitor_or_null(const Absolute_path& job_path) { 
        return file_based_or_null(job_path); 
    }

    xml::Element_ptr new_file_baseds_dom_element(const xml::Document_ptr& doc, const Show_what&) { 
        return doc.createElement("monitors"); 
    }
};


ptr<Monitor_subsystem> new_monitor_subsystem(Scheduler* scheduler) {
    ptr<Standard_monitor_subsystem> result = Z_NEW(Standard_monitor_subsystem(scheduler));
    return +result;
}

}}
