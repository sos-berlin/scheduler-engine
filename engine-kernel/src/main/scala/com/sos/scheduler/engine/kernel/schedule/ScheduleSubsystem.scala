package com.sos.scheduler.engine.kernel.schedule

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{Schedule_subsystemC, ScheduleC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}
import com.sos.scheduler.engine.data.folder.FolderPath

@Singleton
final class ScheduleSubsystem @Inject private(
  protected[this] val cppProxy: Schedule_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue)
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
