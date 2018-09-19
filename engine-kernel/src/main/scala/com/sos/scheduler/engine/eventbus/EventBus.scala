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

  def publish(e: AnyKeyedEvent): Unit
}

