package com.sos.scheduler.engine.kernel.schedule

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{Schedule_subsystemC, ScheduleC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
final class ScheduleSubsystem @Inject private(
  protected[this] val cppProxy: Schedule_subsystemC,
  implicit protected[this] val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  type MySubsystem = ScheduleSubsystem
  type MyFileBased = Schedule
  type MyFile_basedC = ScheduleC

  val companion = ScheduleSubsystem
}


object ScheduleSubsystem extends FileBasedSubsystem.Companion[ScheduleSubsystem, SchedulePath, Schedule](FileBasedType.schedule, SchedulePath.apply) {
//  final class Type extends SisterType[ScheduleSubsystem, Schedule_subsystemC] {
//    def sister(proxy: Schedule_subsystemC, context: Sister) = new ScheduleSubsystem(proxy)
//  }
}
