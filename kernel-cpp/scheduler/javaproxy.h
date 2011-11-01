#ifndef __SCHEDULER_JAVAPROXY_H_
#define __SCHEDULER_JAVAPROXY_H_

#include "../javaproxy/com__sos__scheduler__engine__kernel__AbstractHasPlatform.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__Platform.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__Scheduler.h"
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
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__Event.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__event__EventSubsystem.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__folder__FileBased.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__Job.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__main__SchedulerStateHandler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__Order.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderId.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderState.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__OrderStateChangedEvent.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__order__UnmodifiableOrder.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__scripting__APIModuleInstance.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__util__StringValue.h"
#include "../javaproxy/com__sos__scheduler__engine__cplusplus__runtime__Sister.h"
#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/java__util__ArrayList.h"
#include "../javaproxy/java__util__List.h"

namespace sos {
namespace scheduler {

typedef ::javaproxy::com::sos::scheduler::engine::kernel::Platform PlatformJ;
typedef ::javaproxy::com::sos::scheduler::engine::kernel::Scheduler SchedulerJ;
typedef ::javaproxy::java::lang::String StringJ;


}} //namespaces

using namespace ::javaproxy::com::sos::scheduler::engine::kernel::cppproxy;

#endif
