package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.kernel.cppproxy.Nested_job_chain_nodeC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp

@ForCpp
final class NestedJobChainNode(
  protected val cppProxy: Nested_job_chain_nodeC,
  protected val injector: Injector)
extends Node {
  override def overview =
    NestedJobChainNodeOverview(
      orderState = orderState,
      nextState = nextState,
      errorState = errorState,
      nestedJobChainPath = nestedJobChainPath)

  def nestedJobChainPath =
    JobChainPath(cppProxy.nested_job_chain_path)
}


object NestedJobChainNode {
  final class Type extends SisterType[NestedJobChainNode, Nested_job_chain_nodeC] {
    def sister(proxy: Nested_job_chain_nodeC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new NestedJobChainNode(proxy, injector)
    }
  }
}
