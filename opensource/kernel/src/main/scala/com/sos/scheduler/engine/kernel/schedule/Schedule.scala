package com.sos.scheduler.engine.kernel.schedule

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.cppproxy.ScheduleC
import com.sos.scheduler.engine.kernel.filebased.FileBased

final class Schedule private(
  protected[this] val cppProxy: ScheduleC)
extends FileBased {

  type Path = SchedulePath

  def stringToPath(o: String) = SchedulePath(o)

  def fileBasedType = FileBasedType.schedule

  def onCppProxyInvalidated() = {}
}

@ForCpp
private object Schedule {
  final class Type extends SisterType[Schedule, ScheduleC] {
    def sister(proxy: ScheduleC, context: Sister) = new Schedule(proxy)
  }
}
