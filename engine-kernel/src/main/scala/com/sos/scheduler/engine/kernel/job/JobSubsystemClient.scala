package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
class JobSubsystemClient @Inject private(
  protected val subsystem: JobSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  override type ThisPath = JobPath

  def jobView[V <: JobView: JobView.Companion](path: JobPath): V =
    inSchedulerThread {
      job(path).view[V]
    }

  @deprecated("Avoid direct access to C++ near objects")
  def job(path: JobPath): Job = fileBased(path)

  @deprecated("Avoid direct access to C++ near objects")
  def jobOption(path: JobPath): Option[Job] = fileBasedOption(path)
}
