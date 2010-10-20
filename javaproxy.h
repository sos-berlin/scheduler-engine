#ifndef __SCHEDULER_JAVAPROXY_H_
#define __SCHEDULER_JAVAPROXY_H_

#include "../javaproxy/com__sos__scheduler__kernel__core__AbstractHasPlatform.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__Platform.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__Scheduler.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__JobC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Job_chainC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Job_subsystemC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__NodeC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__OrderC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Order_queueC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Order_queue_nodeC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Order_subsystemC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__Prefix_logC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__event__Event.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__event__EventSubsystem.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__job__Job.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__order__Order.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__order__OrderId.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__order__OrderState.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__order__OrderStateChangeEvent.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__order__StringValue.h"
#include "../javaproxy/com__sos__scheduler__kernel__core__scripting__Module.h"
#include "../javaproxy/com__sos__scheduler__kernel__cplusplus__runtime__Sister.h"
#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/java__util__ArrayList.h"

namespace sos {
namespace scheduler {

typedef ::javaproxy::com::sos::scheduler::kernel::core::Platform PlatformJ;
typedef ::javaproxy::com::sos::scheduler::kernel::core::Scheduler SchedulerJ;
typedef ::javaproxy::java::lang::String StringJ;


}} //namespaces

using namespace ::javaproxy::com::sos::scheduler::kernel::core::cppproxy;

#endif
