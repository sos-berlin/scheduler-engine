#ifndef __SCHEDULER_MONITOR_SUBSYSTEM_H
#define __SCHEDULER_MONITOR_SUBSYSTEM_H

#include "spooler.h"

namespace sos {
namespace scheduler {


struct Monitor_subsystem: Object, 
                          file_based_subsystem<Monitor>,
                          javabridge::has_proxy<Monitor_subsystem>
{
    protected: Monitor_subsystem(Spooler*);

    public: virtual ptr<Monitor_folder> new_monitor_folder(Folder*) = 0;

    public: Monitor* monitor(const Absolute_path& job_path) { 
        return file_based(job_path); 
    }

    public: Monitor* monitor_or_null(const Absolute_path& job_path) { 
        return file_based_or_null(job_path); 
    }

    public: xml::Element_ptr new_file_baseds_dom_element(const xml::Document_ptr& doc, const Show_what&) { 
        return doc.createElement("monitors"); 
    }

    public: jobject java_sister() { 
        return javabridge::has_proxy<Monitor_subsystem>::java_sister(); 
    }
};

ptr<Monitor_subsystem> new_monitor_subsystem(Scheduler*);

}}
#endif
