package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.jobchain.{JobChainNodePersistentState, JobChainPath}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.entities.{JobChainNodeEntity, JobChainNodeEntityConverter}
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManager
import scala.collection.JavaConversions._

@Singleton
final class HibernateJobChainNodeStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId)
extends AbstractHibernateStore[JobChainNodePersistentState, JobChainNodeEntity]
with JobChainNodeEntityConverter {

  def fetchAll(jobChainPath: JobChainPath)(implicit em: EntityManager) =
    (TypedBoundQuery("select n from JobChainNodeEntity n where ", classOf[JobChainNodeEntity]) ++ nodeCondition(jobChainPath)).getResultList map toObject

  def deleteAll(jobChainPath: JobChainPath)(implicit em: EntityManager): Unit = {
    (BoundQuery("delete from JobChainNodeEntity n where ") ++ nodeCondition(jobChainPath)).executeUpdate()
  }

  def nodeCondition(jobChainPath: JobChainPath) = BoundQuery(
    "n.schedulerId = :schedulerId and n.clusterMemberId = :clusterMemberId and n.jobChainPath = :jobChainPath",
    "schedulerId" -> schedulerIdDBString,
    "clusterMemberId" -> clusterMemberIdDBString,
    "jobChainPath" -> jobChainPath.withoutStartingSlash)
}
