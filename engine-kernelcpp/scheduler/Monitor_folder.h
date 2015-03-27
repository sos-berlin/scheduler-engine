#ifndef __SCHEDULER_MONITOR_FOLDER_H
#define __SCHEDULER_MONITOR_FOLDER_H

namespace sos {
namespace scheduler {

struct Monitor_folder: typed_folder<Monitor>
{
    public: Monitor_folder(Folder*);

    public: Monitor* monitor(const Absolute_path& job_path) { 
        return file_based(job_path); 
    }

    public: Monitor* monitor_or_null(const Absolute_path& job_path) { 
        return file_based_or_null(job_path); 
    }

    public: xml::Element_ptr new_file_baseds_dom_element(const xml::Document_ptr& doc, const Show_what&) { 
        return doc.createElement("monitors"); 
    }
 
    public: xml::Element_ptr new_dom_element(const xml::Document_ptr& doc, const Show_what&) {
        return doc.createElement("monitors"); 
    }
};

}}
#endif
