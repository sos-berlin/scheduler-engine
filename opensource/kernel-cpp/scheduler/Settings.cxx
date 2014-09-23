#include "spooler.h"
#include "../kram/licence.h"

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------------role_to_string

static string role_to_string(Settings::Role role) {
    switch (role) {
        case Settings::role_scheduler: return "scheduler";
        case Settings::role_agent: return "agent";
        default: return "ROLE-" + as_string((int)role);
    }
}

//-------------------------------------------------------------------------------Settings::Settings

Settings::Settings()
: 
    _zero_(this+1),
    _keep_order_content_on_reschedule(true),
    _max_length_of_blob_entry(INT_MAX),
    _supervisor_configuration_polling_interval(15 * 60),
    _cluster_restart_after_emergency_abort(true),
    _use_old_microscheduling_for_jobs(false), // Funktioniert nicht in allen Fällen
    _use_old_microscheduling_for_tasks(true)  // JS-1140 Fehlerbehandlung in Task mit Async_operation-Kindern funktioniert nicht (weil Task async_finished() pollt)
{
    if (SOS_LICENCE(licence_scheduler)) 
        _roles.insert(role_scheduler);
    if (SOS_LICENCE(licence_scheduler_agent))
        _roles.insert(role_agent);
}

//------------------------------------------------------------------------------Settings::~Settings

Settings::~Settings() {}

//---------------------------------------------------------------------------Settings::set_defaults

void Settings::set_defaults(Spooler* spooler) {
    if (_freezed) z::throw_xc("FREEZED", Z_FUNCTION);
    if (_html_dir.empty())  
        _html_dir = spooler->home_directory() + "/operations_gui";                  
}

//---------------------------------------------------------------------------Settings::set_defaults

void Settings::set_from_variables(const Com_variable_set& p) {
    if (!_freezed) {
        _keep_order_content_on_reschedule = p.get_bool("scheduler.order.keep_order_content_on_reschedule", _keep_order_content_on_reschedule);
        _max_length_of_blob_entry = max(0, p.get_int("scheduler.max_kbyte_of_db_log_entry", _max_length_of_blob_entry / 1024)) * 1024;
        _order_distributed_balanced = p.get_bool("scheduler.order.distributed.balanced", _order_distributed_balanced);
        _supervisor_configuration_polling_interval = p.get_int("scheduler.configuration.client.polling_interval", _supervisor_configuration_polling_interval);
        _cluster_restart_after_emergency_abort = p.get_bool("scheduler.cluster.restart_after_emergency_abort", _cluster_restart_after_emergency_abort);
    }
    _use_old_microscheduling_for_jobs = p.get_bool("scheduler.old_microscheduling.enable_for_jobs", _use_old_microscheduling_for_jobs);
    _use_old_microscheduling_for_tasks = p.get_bool("scheduler.old_microscheduling.enable_for_tasks", _use_old_microscheduling_for_tasks);
}

//------------------------------------------------------------------------------------Settings::set

void Settings::set(int number, const string& value) {
    if (_freezed) z::throw_xc("FREEZED", Z_FUNCTION);

    // number wird in Java SettingName festgelegt.
    switch ((Setting_name)number) {
        case setting_db_name: 
            _db_name = value; 
            break;
        case setting_job_java_class_pass:
            _job_java_classpath = value; 
            break;
        case setting_html_dir:
            _html_dir = value;
            break;
        case setting_job_java_options:
            _job_java_options = value;
            break;
        case setting_use_java_persistence:
            _use_java_persistence = as_bool(value);
            break;
        case setting_order_distributed_balanced:
            _order_distributed_balanced = as_bool(value);
            break;
        case setting_supervisor_configuration_polling_interval:
            _supervisor_configuration_polling_interval = as_int(value);
            break;
        case setting_cluster_restart_after_emergency_abort:
            _cluster_restart_after_emergency_abort = as_bool(value);
            break;
        case setting_use_old_microscheduling_for_jobs:
            _use_old_microscheduling_for_jobs = as_bool(value);
            break;
        case setting_use_old_microscheduling_for_tasks:
            _use_old_microscheduling_for_tasks = as_bool(value);
            break;
        case setting_always_create_database_tables:
            _always_create_database_tables = as_bool(value);
            break;
        case setting_roles: {
            _roles.clear();
            vector<string> role_strings = vector_split("( +)|( *, *)", value);
            Z_FOR_EACH(vector<string>, role_strings, i) {
                string role_string = *i;
                if (role_string == "scheduler") {
                    if (!SOS_LICENCE(licence_scheduler)) sos::throw_xc("SOS-1000", "Scheduler");
                    _roles.insert(role_scheduler);
                } else
                if (role_string == "agent") {
                    if (!SOS_LICENCE(licence_scheduler_agent)) sos::throw_xc("SOS-1000", "Agent");
                    _roles.insert(role_agent);
                } else
                    z::throw_xc("SCHEDULER-391", "roles", role_string, "scheduler, agent");
            }
            break;
        }
        default:
            z::throw_xc("UNKNOWN_SETTING", number);
    }
}

//------------------------------------------------------------------------------------Settings::get

string Settings::get(int number) const {
    // number wird in Java SettingName festgelegt.

    switch (number) {
        case 1: return _db_name;
        case 2: return _job_java_classpath;
        case 3: return _html_dir;
        default:
            z::throw_xc("UNKNOWN_SETTING", number);
    }
}

//---------------------------------------------------------------------------Settings::require_role

void Settings::require_role(Role role) const {
    if (_roles.find(role) == _roles.end())
        z::throw_xc("SCHEDULER-487", role_to_string(role));
}

void Settings::require_role(Role role, const string& info) const {
    if (_roles.find(role) == _roles.end())
        z::throw_xc("SCHEDULER-487", role_to_string(role), info);
}

}} //namespace sos::scheduler
