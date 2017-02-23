package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.jobchain.EndNodeOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.End_nodeC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class EndNode(
  protected[kernel] val cppProxy: End_nodeC,
  protected val injector: Injector)
extends Node {

  private[kernel] override def overview = EndNodeOverview(nodeKey.jobChainPath, nodeKey.nodeId)

  protected val schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]
}

object EndNode {
  final class Type extends SisterType[EndNode, End_nodeC] {
    def sister(proxy: End_nodeC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new EndNode(proxy, injector)
    }
  }
}
