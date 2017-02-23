package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.job.{JobPath, TaskPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.entities.TaskEntityConverter.toDBString
import com.sos.scheduler.engine.persistence.entities.{TaskEntity, TaskEntityConverter}
import javax.inject.Inject
import javax.persistence.{EntityManager, EntityManagerFactory}
import scala.collection.JavaConversions._

@javax.inject.Singleton
final class HibernateTaskStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[TaskPersistentState, TaskEntity]
with TaskEntityConverter {

  private val queryString = "select t from TaskEntity t"+
    " where t.schedulerId = :schedulerId"+
    " and t.clusterMemberId "+(if (clusterMemberIdDBString == null) "is null" else "= :clusterMemberId")+
    " and t.jobPath = :jobPath"+
    " order by t.taskId"

  def fetchByJobOrderedByTaskId(jobPath: JobPath)(implicit em: EntityManager) = {
    val q = em.createQuery(queryString, classOf[TaskEntity])
    q.setParameter("schedulerId", schedulerIdDBString)
    if (clusterMemberIdDBString != null)
      q.setParameter("clusterMemberId", clusterMemberIdDBString)
    q.setParameter("jobPath", toDBString(jobPath))
    q.getResultList map toObject
  }
}
