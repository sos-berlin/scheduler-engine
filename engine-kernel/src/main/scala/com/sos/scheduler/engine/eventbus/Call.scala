package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}

private[eventbus] final case class Call private[eventbus](keyedEvent: KeyedEvent[Event], subscription: EventSubscription) {

  private[eventbus] def apply(): Unit =
    subscription.handleEvent(keyedEvent)
}
