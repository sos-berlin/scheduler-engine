package com.sos.scheduler.engine.data.order.jobchain

import com.sos.scheduler.engine.data.base.HasKey
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderState

final case class JobChainNodePersistentState(
    jobChainPath: JobChainPath,
    state: OrderState,
    action: JobChainNodeAction)
extends HasKey[JobChainNodePersistentStateKey] {

  def key = JobChainNodePersistentStateKey(jobChainPath, state)
}
