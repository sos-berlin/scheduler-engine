package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import java.util.function.Consumer

/**
  * @author Joacim Zschimmer
  */
final class JavaEventSubscription[E <: Event](val eventClass: Class[E], handleEvent: Consumer[KeyedEvent[E]])
extends EventSubscription {

  def handleEvent(e: AnyKeyedEvent): Unit = handleEvent.accept(e.asInstanceOf[KeyedEvent[E]])
}
