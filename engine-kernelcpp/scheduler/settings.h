#ifndef __SCHEDULER_CONFIGURATION_H__
#define __SCHEDULER_CONFIGURATION_H__

namespace sos {
namespace scheduler {

struct Spooler;

namespace com_objects {
    struct Com_variable_set;
}

enum Setting_name {
    // Dieselben Codes wir in CppSettings.java

    /** Wie -db= */
    setting_db_name = 1,

    /** Erweitert den Class-Path für einen Java-Job. */
    setting_job_java_class_pass = 2,

    /** Default für factory.ini [spooler] html_dir */
    setting_html_dir = 3,

    /** Grundeinstellung der Java-Optionen für alle Jobs. */
    setting_job_java_options = 4,

    /** Datenbank über neue Java-Schnittstelle statt über die alte Hostware. */
    setting_use_java_persistence = 5,

    setting_order_distributed_balanced = 7,

    setting_supervisor_configuration_polling_interval = 8,

    setting_cluster_restart_after_emergency_abort = 9,

    setting_use_old_microscheduling_for_jobs = 10,

    setting_use_old_microscheduling_for_tasks = 11,

    setting_always_create_database_tables = 12,
    
    setting_roles = 13,

    setting_http_port = 14,

    settings_remote_scheduler_connect_retry_delay = 15,

    setting_web_directory = 16,   // For JettyPlugin
};


struct Settings : z::Object, z::javabridge::has_proxy<Settings> {
    enum Role {
        role_scheduler,         // Schedules Jobs, needs database
        role_agent
    };

                                Settings                    ();
    virtual                    ~Settings                    ();
    void                        set_defaults                (Spooler*);
    void                        set_from_variables          (const com_objects::Com_variable_set&);
    void                        set                         (int number, const string& value);
    
    bool has_role_scheduler() const {
        return _roles.find(role_scheduler) != _roles.end();
    }

    void require_role(Role) const;
    void require_role(Role, const string& info) const;

    void freeze() { 
        _freezed = true; 
        if (_use_old_microscheduling_for_jobs) {
            Z_LOG2("scheduler", "_use_old_microscheduling_for_jobs\n");
        }
    }

    bool is_freezed() const {
        return _freezed; 
    }

    ArrayListJ messageTexts() const;

    string installed_licence_keys_string() const;

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
    bool                       _use_old_microscheduling_for_jobs;
    bool                       _use_old_microscheduling_for_tasks;
    bool                       _always_create_database_tables;  // For tests only to suppress java error messages
    std::set<Role>             _roles;
    int                        _http_port;
    int _remote_scheduler_connect_retry_delay;
    string                     _web_directory;    // For JettyPlugin
    int                        _classic_agent_keep_alive_duration;
};

}} //namespace sos::scheduler

#endif
