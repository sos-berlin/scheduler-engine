package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainPersistentState}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.{JobChainEntityConverter, JobChainEntity}
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
final class HibernateJobChainStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[JobChainPersistentState, JobChainPath, JobChainEntity]
with JobChainEntityConverter