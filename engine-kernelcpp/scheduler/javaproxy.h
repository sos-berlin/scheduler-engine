#ifndef __SCHEDULER_JAVAPROXY_H_
#define __SCHEDULER_JAVAPROXY_H_

#include "../javaproxy/java__lang__Class.h"
#include "../javaproxy/com__google__inject__Injector.h"
#include "../javaproxy/com__sos__scheduler__engine__cplusplus__runtime__Sister.h"
#include "../javaproxy/com__sos__scheduler__engine__data__job__TaskPersistentState.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__Scheduler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__CppEventFactory.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__JobC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Job_chainC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Job_subsystemC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__OrderC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Order_queueC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Prefix_logC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__EventSubsystem.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__filebased__FileBased.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__http__SchedulerHttpRequest.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__http__SchedulerHttpResponse.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__Job.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__Task.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__log__PrefixLog.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__EndNode.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__JobChain.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__JobNode.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__NestedJobChainNode.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__Node.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__SimpleJobNode.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__jobchain__SinkNode.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__Order.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderCallback.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderSubsystem.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__UnmodifiableOrder.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__plugin__PluginSubsystem.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__processclass__agent__StartResult.h"
#include "../javaproxy/com__sos__scheduler__engine__main__SchedulerControllerBridge.h"
#include "../javaproxy/java__util__ArrayList.h"
#include "../javaproxy/java__util__List.h"
#include "../javaproxy/java__lang__Boolean.h"
#include "../javaproxy/java__lang__Object.h"
#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/org__joda__time__Duration.h"
#include "../javaproxy/org__joda__time__ReadableInstant.h"
#include "../javaproxy/scala__Option.h"
#include "../javaproxy/scala__util__Try.h"

namespace sos {
namespace scheduler {

typedef ::javaproxy::com::google::inject::Injector InjectorJ;
typedef ::javaproxy::com::sos::scheduler::engine::data::job::TaskPersistentState TaskPersistentStateJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::Scheduler SchedulerJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::event::CppEventFactory CppEventFactoryJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::http::SchedulerHttpRequest SchedulerHttpRequestJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::http::SchedulerHttpResponse SchedulerHttpResponseJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::log::PrefixLog PrefixLogJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::job::Job JobJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::job::Task TaskJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::Order OrderJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::OrderSubsystem OrderSubsystemJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::OrderCallback OrderCallbackJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::EndNode EndNodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::JobChain JobChainJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::JobNode JobNodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::NestedJobChainNode NestedJobChainNodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::Node NodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::SimpleJobNode SimpleJobNodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::order::jobchain::SinkNode SinkNodeJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::plugin::PluginSubsystem PluginSubsystemJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::processclass::agent::StartResult StartResultJ;

typedef ::javaproxy::java::lang::Boolean BooleanJ;
typedef ::javaproxy::java::lang::Object ObjectJ;
typedef ::javaproxy::java::lang::String StringJ;
typedef ::javaproxy::java::util::ArrayList ArrayListJ;
typedef ::javaproxy::java::util::List ListJ;
typedef ::javaproxy::scala::util::Try TryJ;

}} //namespaces

using namespace ::javaproxy::com::sos::scheduler::engine::kernel::cppproxy;

#endif
