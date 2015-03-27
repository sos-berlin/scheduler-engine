#ifndef __MODULE_MONITOR_H
#define __MODULE_MONITOR_H

namespace sos {
namespace scheduler {

struct Module_monitor : Object
{
    private: Fill_zero _zero_;
    public: string _monitor_name;
    public: int _ordering;
    private: ptr<Module> _module;

    public: Module_monitor();
    public: Module_monitor(Module* module);
    public: Module_monitor(const string& name, Module* module);

    public: void set_dom(const xml::Element_ptr&);
    public: void initialize();

    public: int ordering() const {
        return _ordering;
    }

    public: string monitor_name() const { 
        return _monitor_name;
    }

    public: ptr<Module_instance> create_module_instance() const;

    public: Module* module() const {
        return _module;
    }
    
    public: string obj_name() const { 
        return S() << "Script_monitor " << monitor_name(); 
    }
};

}} // namespaces

#endif
