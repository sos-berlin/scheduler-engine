package com.sos.scheduler.engine.kernel.job

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.folder.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}
import javax.persistence.EntityManagerFactory

@ImplementedBy(classOf[CppJobSubsystem])
trait JobSubsystem
extends FileBasedSubsystem {

  type Path = JobPath

  def job(path: JobPath): Job

  def names: Seq[String]

  def visibleNames: Seq[String]

  private[job] def entityManagerFactory: EntityManagerFactory
  private[job] def jobStore: HibernateJobStore
  private[job] def taskStore: HibernateTaskStore
}
