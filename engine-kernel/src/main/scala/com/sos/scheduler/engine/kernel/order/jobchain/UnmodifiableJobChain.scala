package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.kernel.job.Job

trait UnmodifiableJobChain {

  //def name: String

  //def path: JobChainPath

  private[order] def refersToJob(o: Job): Boolean
}
