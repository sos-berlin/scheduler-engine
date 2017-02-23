package com.sos.scheduler.engine.kernel.job

import com.google.inject.Injector
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC
import com.sos.scheduler.engine.kernel.job.CppJobSubsystem._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobStore, HibernateTaskStore}
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
private[kernel] final class CppJobSubsystem @Inject private(
  protected[this] val cppProxy: Job_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends JobSubsystem {

  private[job] lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private[job] lazy val jobStore = injector.instance[HibernateJobStore]
  private[job] lazy val taskStore = injector.instance[HibernateTaskStore]

  private[kernel] override def overview: JobSubsystemOverview = {
    val superOverview = super.overview
    val jobInfos = paths map jobInfo
    JobSubsystemOverview(
      fileBasedType = superOverview.fileBasedType,
      count = superOverview.count,
      fileBasedStateCounts = superOverview.fileBasedStateCounts,
      jobStateCounts = (jobInfos map { _.jobState }).countEquals,
      waitingForProcessClassCount = jobInfos count { _.waitingForProcessClass })
  }

  private def jobInfo(path: JobPath) = {
    val j = job(path)
    JobInfo(j.fileBasedState, j.state, j.waitingForProcessClass)
  }
}

object CppJobSubsystem {
  private case class JobInfo(fileBasedState: FileBasedState, jobState: JobState, waitingForProcessClass: Boolean)
}
