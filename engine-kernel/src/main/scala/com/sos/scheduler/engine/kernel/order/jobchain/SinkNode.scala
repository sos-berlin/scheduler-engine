package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.SinkNodeOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.Sink_nodeC
import com.sos.scheduler.engine.kernel.order.OrderQueue
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class SinkNode(
  protected[kernel] val cppProxy: Sink_nodeC,
  protected val injector: Injector)
extends JobNode {

  protected implicit val schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]
  lazy val jobPath = JobPath(inSchedulerThread { cppProxy.job_path })

  private[order] def processClassPathOption = jobChain.fileWatchingProcessClassPathOption

  private[kernel] def overview = SinkNodeOverview(nodeKey.jobChainPath, nodeKey.nodeId, nextNodeId, errorNodeId, jobPath, action, orderCount, obstacles)

  private[kernel] def orderQueue: OrderQueue = cppProxy.order_queue.getSister

  private[kernel] def isDeletingFile: Boolean = cppProxy.file_order_sink_remove

  private[kernel] def moveFileTo: String = cppProxy.file_order_sink_move_to
}

object SinkNode {
  object Type extends SisterType[SinkNode, Sink_nodeC] {
    def sister(proxy: Sink_nodeC, context: Sister): SinkNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new SinkNode(proxy, injector)
    }
  }
}
