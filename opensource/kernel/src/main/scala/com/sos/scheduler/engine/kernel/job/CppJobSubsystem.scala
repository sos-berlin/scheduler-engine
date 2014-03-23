package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC
import com.sos.scheduler.engine.kernel.filebased.FileBasedState
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateTaskStore, HibernateJobStore}
import javax.inject.{Singleton, Inject}
import javax.persistence.EntityManagerFactory

@Singleton
final class CppJobSubsystem @Inject private(
  protected[this] val cppProxy: Job_subsystemC,
  implicit protected[this] val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  injector: Injector)
extends JobSubsystem {

  private[job] lazy val entityManagerFactory = injector.apply[EntityManagerFactory]
  private[job] lazy val jobStore = injector.apply[HibernateJobStore]
  private[job] lazy val taskStore = injector.apply[HibernateTaskStore]

  override def overview: JobSubsystemOverview = {
    case class JobInfo(fileBasedState: FileBasedState, jobState: JobState)
    def jobInfo(path: JobPath) = job(path) match { case j ⇒ JobInfo(j.fileBasedState, j.state) }
    val (superOverview, jobInfos) = inSchedulerThread { (super.overview, paths map jobInfo) }
    JobSubsystemOverview(
      fileBasedType = superOverview.fileBasedType,
      count = superOverview.count,
      fileBasedStateCounts = superOverview.fileBasedStateCounts,
      jobStateCounts = (jobInfos map { _.jobState }).countEquals
    )
  }
}
