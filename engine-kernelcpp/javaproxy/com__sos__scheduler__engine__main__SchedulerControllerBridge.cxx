// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__main__SchedulerControllerBridge.h"
#include "java__lang__Object.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace main { 

struct SchedulerControllerBridge__class : ::zschimmer::javabridge::Class
{
    SchedulerControllerBridge__class(const string& class_name);
   ~SchedulerControllerBridge__class();


    static const ::zschimmer::javabridge::class_factory< SchedulerControllerBridge__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< SchedulerControllerBridge__class > SchedulerControllerBridge__class::class_factory ("com.sos.scheduler.engine.main.SchedulerControllerBridge");

SchedulerControllerBridge__class::SchedulerControllerBridge__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
{}

SchedulerControllerBridge__class::~SchedulerControllerBridge__class() {}




SchedulerControllerBridge::SchedulerControllerBridge(jobject jo) { if (jo) assign_(jo); }

SchedulerControllerBridge::SchedulerControllerBridge(const SchedulerControllerBridge& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    SchedulerControllerBridge::SchedulerControllerBridge(SchedulerControllerBridge&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

SchedulerControllerBridge::~SchedulerControllerBridge() { assign_(NULL); }





::zschimmer::javabridge::Class* SchedulerControllerBridge::java_object_class_() const { return _class.get(); }

::zschimmer::javabridge::Class* SchedulerControllerBridge::java_class_() { return SchedulerControllerBridge__class::class_factory.clas(); }


void SchedulerControllerBridge::Lazy_class::initialize() const {
    _value = SchedulerControllerBridge__class::class_factory.clas();
}


}}}}}}
