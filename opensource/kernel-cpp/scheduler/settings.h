#ifndef __SCHEDULER_CONFIGURATION_H__
#define __SCHEDULER_CONFIGURATION_H__

namespace sos {
namespace scheduler {

struct Spooler;

struct Settings : z::Object, z::javabridge::has_proxy<Settings> {
    virtual                    ~Settings                    ();
    void                        set                         (int number, const string& value);
    void                        set_defaults                (Spooler*);

    string                     _db_name;
    string                     _job_java_class_path;
    string                     _html_dir;
};

}} //namespace sos::scheduler

#endif
