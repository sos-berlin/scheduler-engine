#ifndef __MODULE_MONITORS_H
#define __MODULE_MONITORS_H

namespace sos {
namespace scheduler {


struct Module_monitors : Dependant, Object
{
    struct Monitor_reference : Object {
        static bool less_ordering(const Monitor_reference* a, const Monitor_reference* b) {
            return a->ordering() < b->ordering(); 
        }

        virtual int ordering() const = 0;
        virtual Module_monitor* module_monitor() const = 0;
        virtual string monitor_name() const = 0;
    };

    struct Named_monitor_reference : Monitor_reference {
        private: Absolute_path const _path;
        private: int _ordering;
        private: bool const _ordering_is_defined;
        private: ptr<Module_monitor> _module_monitor;

        public: Named_monitor_reference(const Absolute_path& path, bool ordering_is_defined, int ordering) : 
            _path(path),
            _ordering(ordering),
            _ordering_is_defined(ordering_is_defined)
        {}

        public: void set_module_monitor(Module_monitor* m) {
            _module_monitor = m;
            if (!_ordering_is_defined) _ordering = m->_ordering;
        }

        public: int ordering() const {
            if (!_module_monitor) z::throw_xc(Z_FUNCTION);
            return _ordering_is_defined? _ordering : _module_monitor->_ordering;
        }

        public: string monitor_name() const {
            return _path;
        }

        public: const Absolute_path& path() const {
            return _path;
        }

        public: Module_monitor* module_monitor() const {
            if (!_module_monitor) z::throw_xc(Z_FUNCTION);
            return _module_monitor;
        }
    };

    struct Anonymous_monitor_reference : Monitor_reference {
        private: ptr<Module_monitor> const _module_monitor;
        
        public: Anonymous_monitor_reference(const ptr<Module_monitor>& o) :
            _module_monitor(o)
        {}

        public: int ordering() const {
            return _module_monitor->_ordering;
        }

        public: string monitor_name() const {
            return _module_monitor->monitor_name();
        }

        public: Module_monitor* module_monitor() const {
            return _module_monitor;
        }
    };

    private: Fill_zero _zero_;
    private: Module* const _main_module;
    private: Job* const _job;
    private: typedef stdext::hash_map< string, ptr<Monitor_reference> > Monitor_reference_map;
    private: Monitor_reference_map _monitor_reference_map;
    private: vector<Module_monitor*> _module_monitors;
    private: bool _is_loaded;

    public: Module_monitors(Module*);
    public: void close();
    public: void set_dom(const xml::Element_ptr&);
    public: void initialize();
    public: bool try_load();
    private: bool try_load_named_monitors();
    private: vector<Module_monitor*> prepare_module_monitors() const;
    public: bool on_requisite_loaded(File_based*);
    public: bool on_requisite_to_be_removed(File_based*);

    public: void add_module_monitor(Module_monitor*);

    public: vector<Module_monitor*> module_monitors();

    private: Monitor_reference* monitor_reference_or_null(const string&);

    public: bool is_empty() const { 
        return _monitor_reference_map.empty(); 
    }

    public: bool is_loaded() const {
        return _is_loaded;
    }

    private: Spooler* spooler() const;

    public: Has_log* log();

    public: string obj_name() const {
        return "Module_monitors";
    }
};

}}
#endif
