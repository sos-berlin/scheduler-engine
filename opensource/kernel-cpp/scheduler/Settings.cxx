#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

const int  default_kbyte_of_db_lob_entry           = INT_MAX;

//-------------------------------------------------------------------------------Settings::Settings

Settings::Settings()
: 
    _zero_(this+1) 
{}

//------------------------------------------------------------------------------Settings::~Settings

Settings::~Settings() {}

//---------------------------------------------------------------------------Settings::set_defaults

void Settings::set_defaults(Spooler* spooler) {
    if (_html_dir.empty())  
        _html_dir = spooler->home_directory() + "/operations_gui";                  
}

//---------------------------------------------------------------------------Settings::set_defaults

void Settings::set_from_variables(const Com_variable_set& p) {
    _keep_order_content_on_reschedule = p.get_bool("scheduler.order.keep_order_content_on_reschedule", true);
    _max_length_of_blob_entry = p.get_int("scheduler.max_kbyte_of_db_log_entry", default_kbyte_of_db_lob_entry) * 1024;
    _order_distributed_balanced = p.get_bool("scheduler.order.distributed.balanced", false);
}

//------------------------------------------------------------------------------------Settings::set

void Settings::set(int number, const string& value) {
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
