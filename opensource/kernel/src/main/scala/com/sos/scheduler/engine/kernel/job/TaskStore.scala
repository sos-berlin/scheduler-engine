package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskObject, TaskId}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.TaskEntity
import com.sos.scheduler.engine.persistence.entities.TaskEntity.toFieldValue
import javax.inject.Inject
import javax.persistence.{EntityManager, EntityManagerFactory}
import scala.collection.JavaConversions._

@javax.inject.Singleton
final class TaskStore @Inject()(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
  extends AbstractObjectJPAStore[TaskEntity, TaskObject, TaskId] {

  private val _schedulerId = toFieldValue(schedulerId)
  private val _clusterMemberId = toFieldValue(clusterMemberId)
  private val queryString = "select t from TaskEntity t"+
      " where t._schedulerId = :schedulerId"+
      " and t._clusterMemberId "+(if (_clusterMemberId == null) "is null" else ":clusterMemberId")+
      " and t._jobPath = :jobPath"+
      " order by t._taskId"

  def fetchByJobOrderedByTaskId(jobPath: JobPath)(implicit em: EntityManager) = {
    val q = em.createQuery(queryString, classOf[TaskEntity])
    q.setParameter("schedulerId", _schedulerId)
    if (_clusterMemberId != null)
      q.setParameter("clusterMemberId", _clusterMemberId)
    q.setParameter("jobPath", toFieldValue(jobPath))
    q.getResultList map { _.toObject }
  }

  protected def toEntityKey(taskId: TaskId) = java.lang.Integer.valueOf(taskId.value)

  protected def toEntity(o: TaskObject) = TaskEntity(schedulerId, clusterMemberId, o)

  def toObject(e: TaskEntity) = e.toObject
}