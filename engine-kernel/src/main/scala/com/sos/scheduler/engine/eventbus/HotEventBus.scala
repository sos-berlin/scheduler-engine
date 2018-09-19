package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.HotEventBus.logger
import com.sos.scheduler.engine.eventbus.annotated.HotMethodEventSubscriptionFactory
import java.util.Objects.requireNonNull
import java.util.concurrent.atomic.AtomicReference

final class HotEventBus extends AbstractEventBus {

  protected def factory = HotMethodEventSubscriptionFactory.singleton

  private val currentEvent = new AtomicReference[KeyedEvent[Event]](null)  // This code is not thread-safe! Just in case someone tries.

  def publish(e: KeyedEvent[Event]) =
    if (currentEvent.compareAndSet(null, requireNonNull(e)))
      try for (c ← calls(e)) dispatchCall(c)
      finally currentEvent.set(null)
    else
      logger.error("Ignored", new RuntimeException(s"HotEventBus.publish($e): ignoring this event while handling the event '${currentEvent.get}'"))
}

object HotEventBus {
  private val logger = Logger(getClass)
}
