package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.AnyKeyedEvent

/**
 * @author Joacim Zschimmer
 */
trait EventBus {

  def registerAnnotated(o: EventHandlerAnnotated): Unit

  def unregisterAnnotated(o: EventHandlerAnnotated): Unit

  def register(o: EventSubscription): Unit

  def unregister(o: EventSubscription): Unit

  def publish(e: AnyKeyedEvent): Unit
}

