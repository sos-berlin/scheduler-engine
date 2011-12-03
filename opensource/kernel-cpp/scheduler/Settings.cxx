#include "spooler.h"

namespace sos {
namespace scheduler {

//------------------------------------------------------------------------------Settings::~Settings

Settings::~Settings() {}

//------------------------------------------------------------------------------------Settings::set

void Settings::set(int number, const string& value) {
    // name wird in Java SettingName festgelegt.

    switch (number) {
        case 1: 
            _db_name = value; 
            break;
        case 2: 
            _job_java_class_path = value; 
            break;
        default:
            z::throw_xc("UNKNOWN_SETTING", number);
    }
}

}} //namespace sos::scheduler
