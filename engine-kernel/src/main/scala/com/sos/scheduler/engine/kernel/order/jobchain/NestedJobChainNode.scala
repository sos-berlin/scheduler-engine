package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NestedJobChainNodeOverview}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.Nested_job_chain_nodeC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class NestedJobChainNode(
  protected[kernel] val cppProxy: Nested_job_chain_nodeC,
  protected val injector: Injector)
extends Node {

  protected val schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]

  private[kernel] override def overview =
    NestedJobChainNodeOverview(
      nodeKey.jobChainPath,
      nodeKey.nodeId,
      nextNodeId = nextNodeId,
      errorNodeId = errorNodeId,
      nestedJobChainPath = nestedJobChainPath)

  private[kernel] def nestedJobChainPath = JobChainPath(cppProxy.nested_job_chain_path)
}

object NestedJobChainNode {
  final class Type extends SisterType[NestedJobChainNode, Nested_job_chain_nodeC] {
    def sister(proxy: Nested_job_chain_nodeC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new NestedJobChainNode(proxy, injector)
    }
  }
}
