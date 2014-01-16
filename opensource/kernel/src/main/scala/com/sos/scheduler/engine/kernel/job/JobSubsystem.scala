package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC
import com.sos.scheduler.engine.kernel.folder.FileBasedSubsystem

final class JobSubsystem(cppproxy: Job_subsystemC)
    extends FileBasedSubsystem {

  def job(path: JobPath): Job =
    cppproxy.job_by_string(path.string).getSister

  def names: Seq[String] =
    fetchNames(visibleOnly = false)

  def visibleNames: Seq[String] =
    fetchNames(visibleOnly = true)

  private def fetchNames(visibleOnly: Boolean): Seq[String] =
    cppproxy.file_based_names(visibleOnly)
}

