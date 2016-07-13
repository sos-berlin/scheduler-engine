package com.sos.scheduler.engine.kernel.schedule

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.cppproxy.ScheduleC
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

final class Schedule private(
  protected[this] val cppProxy: ScheduleC,
  protected val subsystem: ScheduleSubsystem)
extends FileBased {

  type ThisPath = SchedulePath

  def stringToPath(o: String) = SchedulePath(o)

  def fileBasedType = FileBasedType.schedule

  def onCppProxyInvalidated() = {}
}


private object Schedule {
  final class Type extends SisterType[Schedule, ScheduleC] {
    def sister(proxy: ScheduleC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new Schedule(proxy, injector.instance[ScheduleSubsystem])
    }
  }
}
