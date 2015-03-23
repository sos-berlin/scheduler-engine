#ifndef __MODULE_MONITORS_H
#define __MODULE_MONITORS_H

namespace sos {
namespace scheduler {


struct Module_monitors : Object
{
    private: Fill_zero _zero_;
    private: Module* _main_module;
    public: typedef stdext::hash_map< string, ptr<Module_monitor> > Monitor_map;
    public: Monitor_map _monitor_map;

    public: Module_monitors(Module* module) : 
        _zero_(this +1), 
        _main_module(module) 
    {}

    public: void close();
    public: void set_dom(const xml::Element_ptr&);
    public: void initialize();
    
    public: void add_monitor(Module_monitor* monitor) {
        _monitor_map[monitor->name()] = monitor; 
    }

    public: Module_monitor* monitor_or_null(const string&);

    public: bool is_empty() const { 
        return _monitor_map.empty(); 
    }

    public: vector<Module_monitor*> ordered_monitors();

};

}} // namespaces

#endif
