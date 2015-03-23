#ifndef __MODULE_MONITOR_H
#define __MODULE_MONITOR_H

namespace sos {
namespace scheduler {

struct Module_monitor : Object
{
    static bool less_ordering(const Module_monitor* a, const Module_monitor* b)  { 
        return a->_ordering < b->_ordering; 
    }

    private: Fill_zero _zero_;
    public: string _name;
    public: int _ordering;
    public: ptr<Module> _module;

    public: Module_monitor();
    public: void set_dom(const xml::Element_ptr&);

    public: string name() const { 
        return _name; 
    }
    
    public: string obj_name() const { 
        return S() << "Script_monitor " << name(); 
    }
};

}} // namespaces

#endif
