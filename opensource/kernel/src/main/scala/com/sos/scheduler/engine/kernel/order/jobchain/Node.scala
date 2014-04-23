package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.jobchain.{NodeOverview, JobChainPath, JobChainNodeAction, JobChainNodePersistentState}
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI

/** @author Joacim Zschimmer */
@ForCpp
abstract class Node extends Sister {

  protected val cppProxy: NodeCI
  protected def injector: Injector

  def onCppProxyInvalidated() {}

  final def persistentState =
    new JobChainNodePersistentState(jobChainPath, orderState, action)

  def overview: NodeOverview

  final def jobChainPath =
    JobChainPath(cppProxy.job_chain_path)

  final def orderState =
    OrderState(cppProxy.string_order_state)

  final def nextState =
    OrderState(cppProxy.string_next_state)

  final def errorState =
    OrderState(cppProxy.string_error_state)

  final def action =
    JobChainNodeAction.ofCppName(cppProxy.string_action)

  final def action_=(o: JobChainNodeAction) {
    cppProxy.set_action_string(o.toCppName)
  }
}
