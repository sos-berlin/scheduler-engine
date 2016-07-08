package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobStore, HibernateTaskStore}
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
final class CppJobSubsystem @Inject private(
  protected[this] val cppProxy: Job_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  injector: Injector)
extends JobSubsystem {

  private[job] lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private[job] lazy val jobStore = injector.instance[HibernateJobStore]
  private[job] lazy val taskStore = injector.instance[HibernateTaskStore]

  override def overview: JobSubsystemOverview = {
    case class JobInfo(fileBasedState: FileBasedState, jobState: JobState, needsProcess: Boolean)
    def jobInfo(path: JobPath) = job(path) match { case j ⇒ JobInfo(j.fileBasedState, j.state, j.needsProcess) }
    val (superOverview, jobInfos) = inSchedulerThread { (super.overview, paths map jobInfo) }
    JobSubsystemOverview(
      fileBasedType = superOverview.fileBasedType,
      count = superOverview.count,
      fileBasedStateCounts = superOverview.fileBasedStateCounts,
      jobStateCounts = (jobInfos map { _.jobState }).countEquals,
      needProcessCount = jobInfos count { _.needsProcess }
    )
  }
}
