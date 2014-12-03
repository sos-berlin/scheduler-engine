package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder
import java.lang.String

trait UnmodifiableJobChain {

  def name: String

  def path: JobChainPath

  def refersToJob(o: Job): Boolean

  def order(id: OrderId): UnmodifiableOrder
}
