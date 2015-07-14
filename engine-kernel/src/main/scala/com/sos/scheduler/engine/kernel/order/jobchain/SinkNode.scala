package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.SinkNodeOverview
import com.sos.scheduler.engine.kernel.cppproxy.Sink_nodeC
import com.sos.scheduler.engine.kernel.order.OrderQueue
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class SinkNode(
  protected val cppProxy: Sink_nodeC,
  protected val injector: Injector)
extends JobNode {

  def overview = SinkNodeOverview(orderState, nextState, errorState, action, jobPath, orderCount)

  def jobPath: JobPath = JobPath(cppProxy.job_path)

  def orderCount: Int = cppProxy.order_queue.java_order_count()

  def orderQueue: OrderQueue = cppProxy.order_queue.getSister

  def isDeletingFile: Boolean = cppProxy.file_order_sink_remove()

  def moveFileTo: String = cppProxy.file_order_sink_move_to()
}

private object SinkNode {
  final class Type extends SisterType[SinkNode, Sink_nodeC] {
    def sister(proxy: Sink_nodeC, context: Sister): SinkNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new SinkNode(proxy, injector)
    }
  }
}
