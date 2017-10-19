package com.sos.scheduler.engine.taskserver.modules.javamodule

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.taskserver.moduleapi.Module
import com.sos.scheduler.engine.taskserver.spoolerapi.TypedNamedIDispatches
import sos.spooler.{Job_impl, Monitor_impl}

/**
  * @author Joacim Zschimmer
  */
trait ApiModule extends Module {

  def newJobInstance(namedIDispatches: TypedNamedIDispatches, stderrLogLevel: SchedulerLogLevel): Job_impl

  def newMonitorInstance(namedIDispatches: TypedNamedIDispatches, stderrLogLevel: SchedulerLogLevel): Monitor_impl
}
