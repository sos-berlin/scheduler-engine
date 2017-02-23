package com.sos.scheduler.engine.kernel.job.internal

import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.kernel.order.Order
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
trait OrderAsynchronousJob extends AsynchronousJob {

  import task.log

  protected def processFileOrder(order: Order): Future[Boolean]
  protected def messageCodeToString: MessageCodeHandler

  final def step() =
    task.orderOption match {
      case None ⇒
        log.warn(messageCodeToString(MessageCode("SCHEDULER-343")))
        Future.successful(false)
      case Some(order) ⇒ processOrder(order)
    }

  private def processOrder(order: Order): Future[Boolean] = {
    order.filePath match {
      case "" ⇒
        log.warn(messageCodeToString(MessageCode("SCHEDULER-343"), order))
        Future.successful(false)
      case filePath ⇒ processFileOrder(order)
    }
  }
}
