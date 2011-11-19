#ifndef __SCHEDULER_CONFIGURATION_H__
#define __SCHEDULER_CONFIGURATION_H__

namespace sos {
namespace scheduler {

struct Settings : z::Object, z::javabridge::has_proxy<Settings> {
    virtual                    ~Settings                    ();
    void                        set_db_name                 (const string& o)                       { _db_name = o; }

    string                     _db_name;
};

}} //namespace sos::scheduler

#endif
