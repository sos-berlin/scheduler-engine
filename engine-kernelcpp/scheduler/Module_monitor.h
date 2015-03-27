#ifndef __MODULE_MONITOR_H
#define __MODULE_MONITOR_H

namespace sos {
namespace scheduler {

struct Module_monitor : Object
{
    private: Fill_zero _zero_;
    public: Absolute_path _path;
    public: string _name;
    public: int _ordering;
    public: ptr<Module> _module;

    public: Module_monitor();
    public: void set_dom(const xml::Element_ptr&);
    public: void initialize();

    public: string monitor_name() const { 
        return _path.empty()? _name : _path; 
    }
    
    public: string obj_name() const { 
        return S() << "Script_monitor " << monitor_name(); 
    }
};

}} // namespaces

#endif
