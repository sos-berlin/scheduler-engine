package com.sos.scheduler.engine.test

import _root_.scala.concurrent.{Promise, Future}
import _root_.scala.reflect.ClassTag
import com.sos.scheduler.engine.data.folder.{JobChainPath, JobPath}
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, Order}
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.eventbus.{EventSubscription, EventBus}

object SchedulerTestUtils {

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    hasInjector.injector.getInstance(classOf[JobSubsystem]).job(jobPath)

  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).jobChain(jobChainPath)

  def order(key: OrderKey)(implicit hasInjector: HasInjector): Order =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).order(key)
}
