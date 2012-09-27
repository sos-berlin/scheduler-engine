package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.{JobEntityKey, JobEntity}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityTransaction, EntityManager}
import org.slf4j.LoggerFactory

@Singleton
class JobPersistentStateStore @Inject()(
    implicit entityManager: EntityManager, schedulerId: SchedulerId, clusterMemberId: ClusterMemberId) {

  import JobPersistentStateStore._

  implicit def toRichEntityManager(e: EntityManager) = new RichEntityManager(e)

  def tryFetch(jobPath: JobPath) =
    transaction {
      entityManager.findOption[JobEntity](toPersistenceEntityKey(jobPath)) map toPersistentState
    }

  def store(s: JobPersistentState) {
    if (s.isDefault)
      delete(s.jobPath)
    else
      transaction {
        var e = new JobEntity(toPersistenceEntityKey(s.jobPath))
        e.nextStartTime = s.nextStartTimeOption
        e.isStopped = s.isPermanentlyStopped
        entityManager.merge(e)
      }
  }

  def delete(jobPath: JobPath) {
    transaction {
      entityManager.findOption[JobEntity](toPersistenceEntityKey(jobPath)) foreach { e =>
        entityManager.remove(e)
      }
    }
  }

  private def toPersistenceEntityKey(jobPath: JobPath) =
    new JobEntityKey(schedulerId, clusterMemberId, jobPath)
}

object JobPersistentStateStore {
  private val logger = LoggerFactory.getLogger(classOf[JobPersistentStateStore])

  private def toPersistentState(e: JobEntity) = JobPersistentState(e.jobPath.get, e.nextStartTime, e.isStopped)

  def transaction[A](f : => A)(implicit entityManager: EntityManager): A =
    transaction(entityManager.getTransaction, f)

  def transaction[A](transaction: EntityTransaction, f : => A) = {
    transaction.begin()
    try {
      val result = f
      transaction.commit()
      result
    }
    catch { case x: Throwable =>
      try {
        transaction.rollback()
      }
      catch { case xx: Exception =>
        logger.error("Second error while rollback ignored: {}", xx)
        logger.error("Original error: {}", x)
      }
      throw x
    }
  }

  class RichEntityManager(entityManager: EntityManager) {
    def findOption[E](key: AnyRef)(implicit m: Manifest[E]): Option[E] =
      Option(entityManager.find(m.erasure, key).asInstanceOf[E])
  }
}
