package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.folder.{AbsolutePath, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.order.jobchain.{JobChainNodePersistentStateKey, JobChainNodeAction, JobChainNodePersistentState}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait JobChainNodeEntityConverter extends ObjectEntityConverter[JobChainNodePersistentState, JobChainNodePersistentStateKey, JobChainNodeEntity] {
  protected val schedulerId: SchedulerId
  protected val clusterMemberId: ClusterMemberId
  protected lazy val schedulerIdDBString = schedulerIdToDatabase(schedulerId)
  protected lazy val clusterMemberIdDBString = if (clusterMemberId.isEmpty) "-" else clusterMemberId.string

  final def toObject(e: JobChainNodeEntity) = JobChainNodePersistentState(
    new JobChainPath(AbsolutePath.of(e.jobChainPath)),
    new OrderState(e.orderState),
    if (e.action == null) JobChainNodeAction.process else JobChainNodeAction.ofCppName(e.action))

  final def toEntity(o: JobChainNodePersistentState) = {
    val e = new JobChainNodeEntity(toEntityKey(o.key))
    e.action = if (o.action == JobChainNodeAction.process) null else o.action.toCppName
    e
  }

  final def toEntityKey(key: JobChainNodePersistentStateKey) = JobChainNodeEntityKey(
    schedulerIdDBString,
    clusterMemberIdDBString,
    key.jobChainPath.withoutStartingSlash,
    key.state.string)
}
