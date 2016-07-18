package com.sos.scheduler.engine.kernel.schedule

import com.google.inject.Injector
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{ScheduleC, Schedule_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
final class ScheduleSubsystem @Inject private(
  protected[this] val cppProxy: Schedule_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystem = ScheduleSubsystem
  type ThisFileBased = Schedule
  type ThisFile_basedC = ScheduleC

  val description = ScheduleSubsystem
}


object ScheduleSubsystem extends FileBasedSubsystem.AbstractDesription[ScheduleSubsystem, SchedulePath, Schedule] {
  val fileBasedType = FileBasedType.schedule
  val stringToPath = SchedulePath.apply _

//  final class Type extends SisterType[ScheduleSubsystem, Schedule_subsystemC] {
//    def sister(proxy: Schedule_subsystemC, context: Sister) = new ScheduleSubsystem(proxy)
//  }
}
