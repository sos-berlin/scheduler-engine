package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}
import javax.inject.{Singleton, Inject}
import javax.persistence.EntityManagerFactory

@Singleton
final class CppJobSubsystem @Inject private(cppproxy: Job_subsystemC, injector: Injector)
extends JobSubsystem {

  private[job] lazy val entityManagerFactory = injector.apply[EntityManagerFactory]
  private[job] lazy val jobStore = injector.apply[HibernateJobStore]
  private[job] lazy val taskStore = injector.apply[HibernateTaskStore]

  def job(path: JobPath): Job =
    cppproxy.job_by_string(path.string).getSister

  def names: Seq[String] =
    fetchNames(visibleOnly = false)

  def visibleNames: Seq[String] =
    fetchNames(visibleOnly = true)

  private def fetchNames(visibleOnly: Boolean): Seq[String] =
    cppproxy.file_based_names(visibleOnly)
}
