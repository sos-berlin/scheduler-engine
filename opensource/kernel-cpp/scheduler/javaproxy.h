#ifndef __SCHEDULER_JAVAPROXY_H_
#define __SCHEDULER_JAVAPROXY_H_

#include "../javaproxy/com__sos__scheduler__engine__cplusplus__runtime__Sister.h"
#include "../javaproxy/com__sos__scheduler__engine__data__job__TaskId.h"
#include "../javaproxy/com__sos__scheduler__engine__data__job__TaskPersistent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__Scheduler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__CppEventFactory.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__JobC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Job_chainC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Job_subsystemC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__NodeC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__OrderC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Order_queueC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Order_queue_nodeC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Order_subsystemC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Prefix_logC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__EventSubsystem.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__folder__FileBased.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__http__SchedulerHttpRequest.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__http__SchedulerHttpResponse.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__Job.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__log__PrefixLog.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__JobChain.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__Node.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__Order.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__UnmodifiableOrder.h"
#include "../javaproxy/com__sos__scheduler__engine__main__SchedulerControllerBridge.h"
#include "../javaproxy/java__util__ArrayList.h"
#include "../javaproxy/java__lang__Object.h"
#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/org__joda__time__Duration.h"
#include "../javaproxy/org__joda__time__ReadableInstant.h"
#include "../javaproxy/scala__Option.h"

namespace sos {
namespace scheduler {

typedef ::javaproxy::com::sos::scheduler::engine::data::job::TaskPersistent TaskPersistentJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::Scheduler SchedulerJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::event::CppEventFactory CppEventFactoryJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::http::SchedulerHttpRequest SchedulerHttpRequestJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::http::SchedulerHttpResponse SchedulerHttpResponseJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::log::PrefixLog PrefixLogJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::job::Job JobJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::JobChain JobChainJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::Node NodeJ;
typedef ::javaproxy::java::lang::Object ObjectJ;
typedef ::javaproxy::java::lang::String StringJ;

}} //namespaces

using namespace ::javaproxy::com::sos::scheduler::engine::kernel::cppproxy;

#endif
