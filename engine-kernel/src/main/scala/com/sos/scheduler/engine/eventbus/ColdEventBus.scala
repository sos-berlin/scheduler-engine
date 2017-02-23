package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.ColdEventBus._
import com.sos.scheduler.engine.eventbus.annotated.ColdMethodEventSubscriptionFactory
import java.util.concurrent.{LinkedBlockingQueue, TimeUnit}
import scala.collection.JavaConversions._

final class ColdEventBus extends AbstractEventBus with Runnable {

  protected def factory = ColdMethodEventSubscriptionFactory.singleton

  private val callQueue = new LinkedBlockingQueue[Call]

  def publish(e: KeyedEvent[Event]) {
    publish(calls(e))
  }

  private[eventbus] def publish(calls: Iterable[Call]) =
    callQueue.addAll(calls)

  def run() = dispatchEvents(true)

  def dispatchEvents(): Unit = dispatchEvents(false)

  private def dispatchEvents(wait: Boolean) {
    while (true) {
      val call = poll(wait)
      if (call == null)
        return
      logger.trace(s"dispatch $call")
      dispatchCall(call)
    }
  }

  private def poll(wait: Boolean) =
    if (wait)
      callQueue.poll(Long.MaxValue, TimeUnit.DAYS)
    else
      callQueue.poll()
}

object ColdEventBus {
  private val logger = Logger(getClass)
}
