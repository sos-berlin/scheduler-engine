package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.SisterType
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.scheduler.HasInjector

final class JobNode(
  protected val cppProxy: Job_nodeC,
  protected val injector: Injector)
extends OrderQueueNode {

  override def overview =
    JobNodeOverview(
      orderState = orderState,
      nextState = nextState,
      errorState = errorState,
      orderCount = orderCount,
      action = action,
      jobPath = jobPath)

  def jobPath: JobPath =
    JobPath(cppProxy.job_path)

  def getJob: Job =
    cppProxy.job.getSister
}


object JobNode {
  final class Type extends SisterType[JobNode, Job_nodeC] {
    def sister(proxy: Job_nodeC, context: Sister): JobNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new JobNode(proxy, injector)
    }
  }
}
