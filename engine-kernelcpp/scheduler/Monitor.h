#ifndef __SCHEDULER_MONITOR_H
#define __SCHEDULER_MONITOR_H

namespace sos {
namespace scheduler {

struct Monitor_folder;
struct Monitor_subsystem;

struct Monitor : file_based<Monitor, Monitor_folder, Monitor_subsystem>,      
                 javabridge::has_proxy<Monitor>,
                 Object
{
    private: Fill_zero _zero_;
    private: ptr<Module_monitor> _module_monitor;

    public: Monitor(Spooler*);

    STDMETHODIMP_(ULONG) AddRef() {
        return Object::AddRef();
    }

    STDMETHODIMP_(ULONG) Release() {
        return Object::Release();
    }

    public: void set_dom(const xml::Element_ptr&);
    public: bool on_initialize();
    public: bool on_load();
    public: bool on_activate();
    public: bool can_be_removed_now();
    
    public: Module_monitor* module_monitor() const {
        return _module_monitor;
    }

    public: xml::Element_ptr dom_element(const xml::Document_ptr&, const Show_what&);

    using File_based::obj_name;

    public: jobject java_sister() { 
        return javabridge::has_proxy<Monitor>::java_sister(); 
    }
};

}}
#endif
