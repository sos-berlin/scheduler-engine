package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.SimpleJobNodeOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.OrderQueue
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

@ForCpp
final class SimpleJobNode(
  protected[kernel] val cppProxy: Job_nodeC,
  protected val injector: Injector)
extends JobNode {

  protected implicit val schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]
  private lazy val _jobPath = JobPath(cppProxy.job_path)

  def jobPath = inSchedulerThread { _jobPath }

  private[order] def processClassPathOption = jobChain.defaultProcessClassPathOption

  private[kernel] def orderQueue: OrderQueue = cppProxy.order_queue.getSister

  override def toString = s"${getClass.getSimpleName}"   //inSchedulerThread $nodeKey $jobPath"

  override private[kernel] def overview = SimpleJobNodeOverview(
    nodeKey.jobChainPath,
    nodeKey.nodeId,
    nextNodeId = nextNodeId,
    errorNodeId = errorNodeId,
    action = action,
    jobPath = jobPath,
    orderCount = orderCount,
    obstacles = obstacles)

  private[kernel] def getJob: Job = cppProxy.job.getSister
}

object SimpleJobNode {
  final class Type extends SisterType[SimpleJobNode, Job_nodeC] {
    def sister(proxy: Job_nodeC, context: Sister): SimpleJobNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new SimpleJobNode(proxy, injector)
    }
  }
}
