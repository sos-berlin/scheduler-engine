// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#ifndef _JAVAPROXY_COM_SOS_SCHEDULER_ENGINE_KERNEL_CPPPROXY_SPOOLERC_H_
#define _JAVAPROXY_COM_SOS_SCHEDULER_ENGINE_KERNEL_CPPPROXY_SPOOLERC_H_

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/java.h"
#include "../zschimmer/Has_proxy.h"
#include "../zschimmer/javaproxy.h"
#include "../zschimmer/lazy.h"
#include "java__lang__Object.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct DatabaseC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Folder_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct HttpResponseC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Job_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Lock_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Order_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Prefix_logC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Process_class_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Schedule_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct SettingsC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Standing_order_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Task_subsystemC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { struct Variable_setC; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace http { struct SchedulerHttpRequest; }}}}}}}
namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace http { struct SchedulerHttpResponse; }}}}}}}
namespace javaproxy { namespace java { namespace lang { struct Object; }}}
namespace javaproxy { namespace java { namespace lang { struct String; }}}
namespace javaproxy { namespace java { namespace util { struct List; }}}


namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 


struct SpoolerC__class;

struct SpoolerC : ::zschimmer::javabridge::proxy_jobject< SpoolerC >, ::javaproxy::java::lang::Object {
  private:
    static SpoolerC new_instance();  // Not implemented
  public:

    SpoolerC(jobject = NULL);

    SpoolerC(const SpoolerC&);

    #ifdef Z_HAS_MOVE_CONSTRUCTOR
        SpoolerC(SpoolerC&&);
    #endif

    ~SpoolerC();

    SpoolerC& operator=(jobject jo) { assign_(jo); return *this; }
    SpoolerC& operator=(const SpoolerC& o) { assign_(o.get_jobject()); return *this; }
    #ifdef Z_HAS_MOVE_CONSTRUCTOR
        SpoolerC& operator=(SpoolerC&& o) { set_jobject(o.get_jobject()); o.set_jobject(NULL); return *this; }
    #endif

    jobject get_jobject() const { return ::zschimmer::javabridge::proxy_jobject< SpoolerC >::get_jobject(); }

  protected:
    void set_jobject(jobject jo) {
        ::zschimmer::javabridge::proxy_jobject< SpoolerC >::set_jobject(jo);
        ::javaproxy::java::lang::Object::set_jobject(jo);
    }
  public:


    ::zschimmer::javabridge::Class* java_object_class_() const;

    static ::zschimmer::javabridge::Class* java_class_();


  private:
    struct Lazy_class : ::zschimmer::abstract_lazy<SpoolerC__class*> {
        void initialize() const;
    };

    Lazy_class _class;
};


}}}}}}}

#endif
