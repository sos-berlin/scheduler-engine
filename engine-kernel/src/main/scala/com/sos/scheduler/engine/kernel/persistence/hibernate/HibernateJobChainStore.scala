package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.jobchain.JobChainPersistentState
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.entities.{JobChainEntity, JobChainEntityConverter}
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
final class HibernateJobChainStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[JobChainPersistentState, JobChainEntity]
with JobChainEntityConverter
