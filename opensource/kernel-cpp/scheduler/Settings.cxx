#include "spooler.h"

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------Settings::Settings

Settings::Settings()
: 
    _zero_(this+1),
    _keep_order_content_on_reschedule(true),
    _max_length_of_blob_entry(INT_MAX),
    _supervisor_configuration_polling_interval(15 * 60),
    _cluster_restart_after_emergency_abort(true)
{}

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
    switch (number) {
        case 1: 
            _db_name = value; 
            break;
        case 2: 
            _job_java_classpath = value; 
            break;
        case 3:
            _html_dir = value;
            break;
        case 4:
            _job_java_options = value;
            break;
        case 5:
            _use_java_persistence = as_bool(value);
            break;
        case 7:
            _order_distributed_balanced = as_bool(value);
            break;
        case 8:
            _supervisor_configuration_polling_interval = as_int(value);
            break;
        case 9:
            _cluster_restart_after_emergency_abort = as_bool(value);
            break;
        case 10:
            _use_old_microscheduling_for_jobs = as_bool(value);
            break;
        case 11:
            _use_old_microscheduling_for_tasks = as_bool(value);
            break;
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

}} //namespace sos::scheduler
