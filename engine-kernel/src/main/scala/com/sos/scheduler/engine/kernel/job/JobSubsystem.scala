package com.sos.scheduler.engine.kernel.job

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.job.{JobHistoryEntry, JobPath}
import com.sos.scheduler.engine.data.queries.JobQuery
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.JobC
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobStore, HibernateTaskStore}
import javax.persistence.EntityManagerFactory
import scala.collection.immutable.Seq
import scala.concurrent.Future

@ImplementedBy(classOf[CppJobSubsystem])
private[kernel] trait JobSubsystem
extends FileBasedSubsystem {

  type ThisSubsystemClient = JobSubsystemClient
  type ThisSubsystem = JobSubsystem
  type ThisFileBased = Job
  type ThisFile_basedC = JobC
  type Path = JobPath

  val companion = JobSubsystem

  private[kernel] def job(path: JobPath) = inSchedulerThread { fileBased(path) }

  private[kernel] final def jobsBy(query: JobQuery): Vector[Job] =
    fileBasedsBy(query.pathQuery) filter (o ⇒ query.isInState(o.state))

  private[job] def entityManagerFactory: EntityManagerFactory
  private[job] def jobStore: HibernateJobStore
  private[job] def taskStore: HibernateTaskStore

  private[kernel] def jobsHistory(jobPath: JobPath, limit: Int): Future[Seq[JobHistoryEntry]] =
    injector.instance[DatabaseJobs].fetchJobHistory(jobPath, limit)
}


object JobSubsystem
extends FileBasedSubsystem.AbstractCompanion[JobSubsystemClient, JobSubsystem, JobPath, Job] {

  val fileBasedType = FileBasedType.Job
  val stringToPath = JobPath.apply _
}
