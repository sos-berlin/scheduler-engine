package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.kernel.cppproxy.Sink_nodeC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class SinkNode(
  protected val cppProxy: Sink_nodeC,
  protected val injector: Injector)
extends Node {

  override def overview = SinkNodeOverview(orderState)
}

private object SinkNode {
  final class Type extends SisterType[SinkNode, Sink_nodeC] {
    def sister(proxy: Sink_nodeC, context: Sister): SinkNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new SinkNode(proxy, injector)
    }
  }
}
