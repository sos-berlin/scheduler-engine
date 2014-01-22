package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.google.inject.ImplementedBy
import javax.persistence.EntityManagerFactory
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}

@ImplementedBy(classOf[CppJobSubsystem])
trait JobSubsystem {
  def job(path: JobPath): Job

  def names: Seq[String]

  def visibleNames: Seq[String]

  private[job] def entityManagerFactory: EntityManagerFactory
  private[job] def jobStore: HibernateJobStore
  private[job] def taskStore: HibernateTaskStore
}
