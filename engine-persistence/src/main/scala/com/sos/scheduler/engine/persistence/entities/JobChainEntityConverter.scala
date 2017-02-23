package com.sos.scheduler.engine.persistence.entities

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait JobChainEntityConverter extends ObjectEntityConverter[JobChainPersistentState, JobChainPath, JobChainEntity] {
  protected val schedulerId: SchedulerId
  protected val clusterMemberId: ClusterMemberId

  final def toObject(e: JobChainEntity) = JobChainPersistentState(
    JobChainPath("/"+ e.jobChainPath),
    isStopped = e.isStopped)

  final def toEntity(o: JobChainPersistentState) = {
    val e = new JobChainEntity(toEntityKey(o.key))
    e.isStopped = o.isStopped
    e
  }

  final def toEntityKey(path: JobChainPath) = JobChainEntityKey(
    schedulerIdToDatabase(schedulerId),
    if (clusterMemberId.isEmpty) "-" else clusterMemberId.string,
    path.withoutStartingSlash)
}
