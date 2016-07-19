package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread

/**
  * @author Joacim Zschimmer
  */
private trait FileBasedClient {
  // STILL UNUSED

  type ThisFileBased <: FileBased

  protected implicit def schedulerThreadCallQueue: SchedulerThreadCallQueue

  @deprecated("Avoid direct access to C++ near objects")
  /*protected*/ def fileBased: ThisFileBased

  def path = inSchedulerThread { fileBased.path }

  def name = inSchedulerThread { fileBased.name }

  def overview = inSchedulerThread { fileBased.overview }

  def details = inSchedulerThread { fileBased.details }
}
