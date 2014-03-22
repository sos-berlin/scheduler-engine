package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.jobchain.JobChainPath

case class JobChainNodePersistentStateKey(jobChainPath: JobChainPath, state: OrderState)
