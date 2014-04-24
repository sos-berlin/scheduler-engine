package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.Event

/**
 * @author Joacim Zschimmer
 */
trait EventBus {

  def registerAnnotated(o: EventHandlerAnnotated)

  def unregisterAnnotated(o: EventHandlerAnnotated)

  def register(o: EventSubscription)

  def unregister(o: EventSubscription)

  def publish(e: Event)
}

