package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppIgnore
import com.sos.scheduler.engine.kernel.processclass.ProcessClass
import org.w3c.dom

@CppClass(clas = "sos::scheduler::Process_class", directory = "scheduler", include = "spooler.h")
trait Process_classC extends CppProxyWithSister[ProcessClass] with File_basedC[ProcessClass] {

  @CppIgnore
  def cppReference: Long

  def max_processes: Int

  def used_process_count: Int

  def java_dom_element: dom.Element
}

object Process_classC {
  val sisterType = ProcessClass.Type
}
