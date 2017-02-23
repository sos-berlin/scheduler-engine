package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.HotEventBus.logger
import com.sos.scheduler.engine.eventbus.annotated.HotMethodEventSubscriptionFactory
import javax.annotation.Nullable

final class HotEventBus extends AbstractEventBus {

  protected def factory = HotMethodEventSubscriptionFactory.singleton

  @Nullable private var currentEvent: KeyedEvent[Event] = null

  def publish(e: KeyedEvent[Event]) = publish(e, calls(e))

  private[eventbus] def publish(e: KeyedEvent[Event], calls: Iterable[Call]) = {
    if (currentEvent != null)
      handleRecursiveEvent(e)
    else
      dispatchNonrecursiveEvent(e, calls)
  }

  private def handleRecursiveEvent(e: KeyedEvent[Event]): Unit = {
    try
      // Kein log().error(), sonst gibt es wieder eine Rekursion
      sys.error(s"HotEventBus.publish($e): ignoring the event triggered by handling the event '$currentEvent'")
    catch {
      case x: Exception ⇒ logger.error("Ignored", x)
    }
  }

  private def dispatchNonrecursiveEvent(e: KeyedEvent[Event], calls: Iterable[Call]): Unit = {
    currentEvent = e
    try for (c ← calls) dispatchCall(c)
    finally currentEvent = null
  }
}

object HotEventBus {
  private val logger = Logger(getClass)
}
