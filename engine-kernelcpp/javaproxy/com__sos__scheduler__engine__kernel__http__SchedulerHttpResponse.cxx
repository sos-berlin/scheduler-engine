// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__kernel__http__SchedulerHttpResponse.h"
#include "java__lang__Object.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace http { 

struct SchedulerHttpResponse__class : ::zschimmer::javabridge::Class
{
    SchedulerHttpResponse__class(const string& class_name);
   ~SchedulerHttpResponse__class();

    ::zschimmer::javabridge::Method const _onNextChunkIsReady____method;

    static const ::zschimmer::javabridge::class_factory< SchedulerHttpResponse__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< SchedulerHttpResponse__class > SchedulerHttpResponse__class::class_factory ("com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse");

SchedulerHttpResponse__class::SchedulerHttpResponse__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
    ,_onNextChunkIsReady____method(this, "onNextChunkIsReady", "()V"){}

SchedulerHttpResponse__class::~SchedulerHttpResponse__class() {}




SchedulerHttpResponse::SchedulerHttpResponse(jobject jo) { if (jo) assign_(jo); }

SchedulerHttpResponse::SchedulerHttpResponse(const SchedulerHttpResponse& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    SchedulerHttpResponse::SchedulerHttpResponse(SchedulerHttpResponse&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

SchedulerHttpResponse::~SchedulerHttpResponse() { assign_(NULL); }




void SchedulerHttpResponse::onNextChunkIsReady() const {
    ::zschimmer::javabridge::raw_parameter_list<0> parameter_list;
    SchedulerHttpResponse__class* cls = _class.get();
    cls->_onNextChunkIsReady____method.call(get_jobject(), parameter_list);
}


::zschimmer::javabridge::Class* SchedulerHttpResponse::java_object_class_() const { return _class.get(); }

::zschimmer::javabridge::Class* SchedulerHttpResponse::java_class_() { return SchedulerHttpResponse__class::class_factory.clas(); }


void SchedulerHttpResponse::Lazy_class::initialize() const {
    _value = SchedulerHttpResponse__class::class_factory.clas();
}


}}}}}}}