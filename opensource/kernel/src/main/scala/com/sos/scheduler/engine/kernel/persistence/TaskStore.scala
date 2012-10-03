package com.sos.scheduler.engine.kernel.persistence

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskPersistent, TaskId}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.TaskEntityConverter.toFieldValue
import com.sos.scheduler.engine.persistence.entities.{TaskEntityConverter, TaskEntity}
import javax.inject.Inject
import javax.persistence.{EntityManager, EntityManagerFactory}
import scala.collection.JavaConversions._

@javax.inject.Singleton
final class TaskStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractObjectJPAStore[TaskPersistent, TaskId, TaskEntity]
with TaskEntityConverter {

  private val queryString = "select t from TaskEntity t"+
    " where t.schedulerId = :schedulerId"+
    " and t.clusterMemberId "+(if (clusterMemberIdString == null) "is null" else ":clusterMemberId")+
    " and t.jobPath = :jobPath"+
    " order by t.taskId"

  def fetchByJobOrderedByTaskId(jobPath: JobPath)(implicit em: EntityManager) = {
    val q = em.createQuery(queryString, classOf[TaskEntity])
    q.setParameter("schedulerId", schedulerIdString)
    if (clusterMemberIdString != null)
      q.setParameter("clusterMemberId", clusterMemberIdString)
    q.setParameter("jobPath", toFieldValue(jobPath))
    q.getResultList map toObject
  }
}