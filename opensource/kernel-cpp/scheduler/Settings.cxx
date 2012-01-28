#include "spooler.h"

namespace sos {
namespace scheduler {

//------------------------------------------------------------------------------Settings::~Settings

Settings::~Settings() {}

//------------------------------------------------------------------------------------Settings::set

void Settings::set(int number, const string& value) {
    // number wird in Java SettingName festgelegt.

    switch (number) {
        case 1: 
            _db_name = value; 
            break;
        case 2: 
            _job_java_class_path = value; 
            break;
        case 3:
            _html_dir = value;
            break;
        default:
            z::throw_xc("UNKNOWN_SETTING", number);
    }
}

//---------------------------------------------------------------------------Settings::set_defaults

void Settings::set_defaults(Spooler* spooler) {
    if (_html_dir.empty())  
        _html_dir = spooler->home_directory() + "/operations_gui";
}

}} //namespace sos::scheduler
