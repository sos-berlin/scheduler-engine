package com.sos.scheduler.engine.kernel.schedule

import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class ScheduleSubsystemClient @Inject private(
  protected val subsystem: ScheduleSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  def schedule(path: SchedulePath) = fileBased(path)

  def scheduleOption(path: SchedulePath) = fileBasedOption(path)
}
