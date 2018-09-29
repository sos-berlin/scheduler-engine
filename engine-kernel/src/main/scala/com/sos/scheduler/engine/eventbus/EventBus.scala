package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent}

/**
 * @author Joacim Zschimmer
 */
trait EventBus {

  def registerAnnotated(o: EventHandlerAnnotated): Unit

  def unregisterAnnotated(o: EventHandlerAnnotated): Unit

  def subscribe(o: EventSubscription): Unit

  def unsubscribe(o: EventSubscription): Unit

  /** Not thread-safe. Not for plugin usage! */
  def publishJava[E <: Event](e: KeyedEvent[E]): Unit =
    publish(e)

  def publish(e: AnyKeyedEvent): Unit
}
