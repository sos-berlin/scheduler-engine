#ifndef __SCHEDULER_CONFIGURATION_H__
#define __SCHEDULER_CONFIGURATION_H__

namespace sos {
namespace scheduler {

struct Spooler;

namespace com_objects {
    struct Com_variable_set;
}

struct Settings : z::Object, z::javabridge::has_proxy<Settings> {
                                Settings                    ();
    virtual                    ~Settings                    ();
    void                        set_defaults                (Spooler*);
    void                        set_from_variables          (const com_objects::Com_variable_set&);
    void                        set                         (int number, const string& value);
    string                      get                         (int number) const;

    void freeze() { 
        _freezed = true; 
    }

    bool is_freezed() const {
        return _freezed; 
    }

    Fill_zero                  _zero_;
    bool                       _freezed;       // Some, not all, settings can be changed at runtime
    string                     _db_name;
    string                     _job_java_options;
    string                     _job_java_classpath;
    string                     _html_dir;
    bool                       _keep_order_content_on_reschedule;
    int                        _max_length_of_blob_entry;
    bool                       _use_java_persistence;
    bool                       _order_distributed_balanced;
    int                        _supervisor_configuration_polling_interval;
    bool                       _cluster_restart_after_emergency_abort;
    bool                       _always_create_database_tables;  // For tests only to suppress java error messages
};

}} //namespace sos::scheduler

#endif
