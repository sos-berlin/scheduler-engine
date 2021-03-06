// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__kernel__time__TimeZones.h"
#include "java__lang__Object.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace time { 

struct TimeZones__class : ::zschimmer::javabridge::Class
{
    TimeZones__class(const string& class_name);
   ~TimeZones__class();

    ::zschimmer::javabridge::Static_method const _localToUtc__Ljava_lang_String_2J__method;
    ::zschimmer::javabridge::Static_method const _toString__Ljava_lang_String_2ZJ__method;
    ::zschimmer::javabridge::Static_method const _utcToLocal__Ljava_lang_String_2J__method;

    static const ::zschimmer::javabridge::class_factory< TimeZones__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< TimeZones__class > TimeZones__class::class_factory ("com.sos.scheduler.engine.kernel.time.TimeZones");

TimeZones__class::TimeZones__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
    ,_localToUtc__Ljava_lang_String_2J__method(this, "localToUtc", "(Ljava/lang/String;J)J")
    ,_toString__Ljava_lang_String_2ZJ__method(this, "toString", "(Ljava/lang/String;ZJ)Ljava/lang/String;")
    ,_utcToLocal__Ljava_lang_String_2J__method(this, "utcToLocal", "(Ljava/lang/String;J)J"){}

TimeZones__class::~TimeZones__class() {}




TimeZones::TimeZones(jobject jo) { if (jo) assign_(jo); }

TimeZones::TimeZones(const TimeZones& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    TimeZones::TimeZones(TimeZones&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

TimeZones::~TimeZones() { assign_(NULL); }




jlong TimeZones::localToUtc(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p0, jlong p1) {
    ::zschimmer::javabridge::raw_parameter_list<2> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].j = p1;
    TimeZones__class* cls = TimeZones__class::class_factory.clas();
    return cls->_localToUtc__Ljava_lang_String_2J__method.long_call(cls->get_jclass(), parameter_list);
}

::javaproxy::java::lang::String TimeZones::toString(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p0, jboolean p1, jlong p2) {
    ::zschimmer::javabridge::raw_parameter_list<3> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].z = p1;
    parameter_list._jvalues[2].j = p2;
    TimeZones__class* cls = TimeZones__class::class_factory.clas();
    ::javaproxy::java::lang::String result;
    result.steal_local_ref(cls->_toString__Ljava_lang_String_2ZJ__method.jobject_call(cls->get_jclass(), parameter_list));
    return result;
}

jlong TimeZones::utcToLocal(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p0, jlong p1) {
    ::zschimmer::javabridge::raw_parameter_list<2> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].j = p1;
    TimeZones__class* cls = TimeZones__class::class_factory.clas();
    return cls->_utcToLocal__Ljava_lang_String_2J__method.long_call(cls->get_jclass(), parameter_list);
}


::zschimmer::javabridge::Class* TimeZones::java_object_class_() const { return _class.get(); }

::zschimmer::javabridge::Class* TimeZones::java_class_() { return TimeZones__class::class_factory.clas(); }


void TimeZones::Lazy_class::initialize() const {
    _value = TimeZones__class::class_factory.clas();
}


}}}}}}}
