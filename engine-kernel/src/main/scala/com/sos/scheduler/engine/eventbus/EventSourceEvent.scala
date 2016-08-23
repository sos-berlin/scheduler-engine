package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.Event

final case class EventSourceEvent[E <: Event](event: E, eventSource: EventSource)
extends Event {
  type Key = E#Key
}
