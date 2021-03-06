// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__kernel__job__internal__InternalModule.h"
#include "com__google__inject__Injector.h"
#include "com__sos__scheduler__engine__kernel__async__CppCall.h"
#include "com__sos__scheduler__engine__kernel__job__Task.h"
#include "java__lang__Object.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace job { namespace internal { 

struct InternalModule__class : ::zschimmer::javabridge::Class
{
    InternalModule__class(const string& class_name);
   ~InternalModule__class();

    ::zschimmer::javabridge::Method const _addObj__Ljava_lang_Object_2Ljava_lang_String_2__method;
    ::zschimmer::javabridge::Static_method const _apply__Lcom_google_inject_Injector_2Ljava_lang_String_2Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_job_Task_2__method;
    ::zschimmer::javabridge::Method const _begin__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _call__Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _close__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _end__ZLcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _nameExists__Ljava_lang_String_2__method;
    ::zschimmer::javabridge::Method const _release__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _start__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;
    ::zschimmer::javabridge::Method const _step__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method;

    static const ::zschimmer::javabridge::class_factory< InternalModule__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< InternalModule__class > InternalModule__class::class_factory ("com.sos.scheduler.engine.kernel.job.internal.InternalModule");

InternalModule__class::InternalModule__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
    ,_addObj__Ljava_lang_Object_2Ljava_lang_String_2__method(this, "addObj", "(Ljava/lang/Object;Ljava/lang/String;)V")
    ,_apply__Lcom_google_inject_Injector_2Ljava_lang_String_2Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_job_Task_2__method(this, "apply", "(Lcom/google/inject/Injector;Ljava/lang/String;Ljava/lang/String;Lcom/sos/scheduler/engine/kernel/job/Task;)Lcom/sos/scheduler/engine/kernel/job/internal/InternalModule;")
    ,_begin__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "begin", "(Lcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_call__Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "call", "(Ljava/lang/String;Lcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_close__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "close", "(Lcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_end__ZLcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "end", "(ZLcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_nameExists__Ljava_lang_String_2__method(this, "nameExists", "(Ljava/lang/String;)Z")
    ,_release__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "release", "(Lcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_start__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "start", "(Lcom/sos/scheduler/engine/kernel/async/CppCall;)V")
    ,_step__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method(this, "step", "(Lcom/sos/scheduler/engine/kernel/async/CppCall;)V"){}

InternalModule__class::~InternalModule__class() {}




InternalModule::InternalModule(jobject jo) { if (jo) assign_(jo); }

InternalModule::InternalModule(const InternalModule& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    InternalModule::InternalModule(InternalModule&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

InternalModule::~InternalModule() { assign_(NULL); }




void InternalModule::addObj(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::Object >& p0, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p1) const {
    ::zschimmer::javabridge::raw_parameter_list<2> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].l = p1.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_addObj__Ljava_lang_Object_2Ljava_lang_String_2__method.call(get_jobject(), parameter_list);
}

::javaproxy::com::sos::scheduler::engine::kernel::job::internal::InternalModule InternalModule::apply(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::google::inject::Injector >& p0, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p1, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p2, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::job::Task >& p3) {
    ::zschimmer::javabridge::raw_parameter_list<4> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].l = p1.get_jobject();
    parameter_list._jvalues[2].l = p2.get_jobject();
    parameter_list._jvalues[3].l = p3.get_jobject();
    InternalModule__class* cls = InternalModule__class::class_factory.clas();
    ::javaproxy::com::sos::scheduler::engine::kernel::job::internal::InternalModule result;
    result.steal_local_ref(cls->_apply__Lcom_google_inject_Injector_2Ljava_lang_String_2Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_job_Task_2__method.jobject_call(cls->get_jclass(), parameter_list));
    return result;
}

void InternalModule::begin(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_begin__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

void InternalModule::call(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p0, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p1) const {
    ::zschimmer::javabridge::raw_parameter_list<2> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    parameter_list._jvalues[1].l = p1.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_call__Ljava_lang_String_2Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

void InternalModule::close(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_close__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

void InternalModule::end(jboolean p0, const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p1) const {
    ::zschimmer::javabridge::raw_parameter_list<2> parameter_list;
    parameter_list._jvalues[0].z = p0;
    parameter_list._jvalues[1].l = p1.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_end__ZLcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

bool InternalModule::nameExists(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::java::lang::String >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    return 0 != cls->_nameExists__Ljava_lang_String_2__method.bool_call(get_jobject(), parameter_list);
}

void InternalModule::release(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_release__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

void InternalModule::start(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_start__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}

void InternalModule::step(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::async::CppCall >& p0) const {
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    InternalModule__class* cls = _class.get();
    cls->_step__Lcom_sos_scheduler_engine_kernel_async_CppCall_2__method.call(get_jobject(), parameter_list);
}


::zschimmer::javabridge::Class* InternalModule::java_object_class_() const { return _class.get(); }

::zschimmer::javabridge::Class* InternalModule::java_class_() { return InternalModule__class::class_factory.clas(); }


void InternalModule::Lazy_class::initialize() const {
    _value = InternalModule__class::class_factory.clas();
}


}}}}}}}}
