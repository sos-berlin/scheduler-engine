package com.sos.scheduler.engine.kernel.job

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}
import javax.persistence.EntityManagerFactory

@ImplementedBy(classOf[CppJobSubsystem])
trait JobSubsystem
extends FileBasedSubsystem {

  type ThisSubsystem = JobSubsystem
  type ThisFileBased = Job
  type ThisFile_basedC = JobC

  val description = JobSubsystem

  def job(path: JobPath) = fileBased(path)

  private[job] def entityManagerFactory: EntityManagerFactory
  private[job] def jobStore: HibernateJobStore
  private[job] def taskStore: HibernateTaskStore
}


object JobSubsystem extends FileBasedSubsystem.AbstractDesription[JobSubsystem, JobPath, Job] {
  val fileBasedType = FileBasedType.job
  val stringToPath = JobPath.apply _
}
