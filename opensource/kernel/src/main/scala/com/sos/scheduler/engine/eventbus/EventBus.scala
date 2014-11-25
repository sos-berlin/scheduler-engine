package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.Event

/**
 * @author Joacim Zschimmer
 */
trait EventBus {

  def registerAnnotated(o: EventHandlerAnnotated): Unit

  def unregisterAnnotated(o: EventHandlerAnnotated): Unit

  def register(o: EventSubscription): Unit

  def unregister(o: EventSubscription): Unit

  def publish(e: Event): Unit
}

