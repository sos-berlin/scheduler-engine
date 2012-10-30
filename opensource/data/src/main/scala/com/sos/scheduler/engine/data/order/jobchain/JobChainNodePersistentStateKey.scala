package com.sos.scheduler.engine.data.order.jobchain

import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderState

case class JobChainNodePersistentStateKey(jobChainPath: JobChainPath, state: OrderState)
