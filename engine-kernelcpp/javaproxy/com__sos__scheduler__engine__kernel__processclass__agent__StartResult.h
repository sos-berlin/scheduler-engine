// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#ifndef _JAVAPROXY_COM_SOS_SCHEDULER_ENGINE_KERNEL_PROCESSCLASS_AGENT_STARTRESULT_H_
#define _JAVAPROXY_COM_SOS_SCHEDULER_ENGINE_KERNEL_PROCESSCLASS_AGENT_STARTRESULT_H_

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/java.h"
#include "../zschimmer/Has_proxy.h"
#include "../zschimmer/javaproxy.h"
#include "../zschimmer/lazy.h"
#include "java__lang__Object.h"

namespace javaproxy { namespace java { namespace lang { struct Object; }}}
namespace javaproxy { namespace java { namespace lang { struct String; }}}
namespace javaproxy { namespace scala { namespace util { struct Try; }}}


namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace processclass { namespace agent { 


struct StartResult__class;

struct StartResult : ::zschimmer::javabridge::proxy_jobject< StartResult >, ::javaproxy::java::lang::Object {
  private:
    static StartResult new_instance();  // Not implemented
  public:

    StartResult(jobject = NULL);

    StartResult(const StartResult&);

    #ifdef Z_HAS_MOVE_CONSTRUCTOR
        StartResult(StartResult&&);
    #endif

    ~StartResult();

    StartResult& operator=(jobject jo) { assign_(jo); return *this; }
    StartResult& operator=(const StartResult& o) { assign_(o.get_jobject()); return *this; }
    #ifdef Z_HAS_MOVE_CONSTRUCTOR
        StartResult& operator=(StartResult&& o) { set_jobject(o.get_jobject()); o.set_jobject(NULL); return *this; }
    #endif

    jobject get_jobject() const { return ::zschimmer::javabridge::proxy_jobject< StartResult >::get_jobject(); }

  protected:
    void set_jobject(jobject jo) {
        ::zschimmer::javabridge::proxy_jobject< StartResult >::set_jobject(jo);
        ::javaproxy::java::lang::Object::set_jobject(jo);
    }
  public:

    ::javaproxy::java::lang::String agentUri() const;
    ::javaproxy::scala::util::Try result() const;

    ::zschimmer::javabridge::Class* java_object_class_() const;

    static ::zschimmer::javabridge::Class* java_class_();


  private:
    struct Lazy_class : ::zschimmer::abstract_lazy<StartResult__class*> {
        void initialize() const;
    };

    Lazy_class _class;
};


}}}}}}}}

#endif