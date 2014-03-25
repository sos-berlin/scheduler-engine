package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.kernel.cppproxy.End_nodeC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class EndNode(
  protected val cppProxy: End_nodeC,
  protected val injector: Injector)
extends Node {

  override def overview = EndNodeOverview(orderState)
}


object EndNode {
  final class Type extends SisterType[EndNode, End_nodeC] {
    def sister(proxy: End_nodeC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new EndNode(proxy, injector)
    }
  }
}
