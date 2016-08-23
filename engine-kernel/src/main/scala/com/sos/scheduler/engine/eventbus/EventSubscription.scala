package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.base.utils.ScalaUtils.{RichUnitPartialFunction, implicitClass}
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: AnyKeyedEvent]
  def handleEvent(e: AnyKeyedEvent): Unit

  override def toString = s"EventSubscription(${eventClass.getSimpleName})"
}

object EventSubscription {

  def apply[E <: AnyKeyedEvent : ClassTag](handleEvent: PartialFunction[E, Unit]): EventSubscription =
    forClass(implicitClass[E])(handleEvent)

  def forClass[E <: AnyKeyedEvent](clas: Class[E])(handleEvent: PartialFunction[E, Unit]): EventSubscription =
    new Impl(clas, handleEvent)

  private class Impl[E <: AnyKeyedEvent](clas: Class[E], handle: PartialFunction[E, Unit]) extends EventSubscription {
    def eventClass = clas

    def handleEvent(e: AnyKeyedEvent) = {
      val unwrappedEvent = (e match {
        case KeyedEvent(key, EventSourceEvent(event, _)) ⇒ KeyedEvent(key, event)
        case _ ⇒ e
      }).asInstanceOf[E]
      handle.callIfDefined(unwrappedEvent)
    }
  }

  def withSource[E <: Event: ClassTag](f: PartialFunction[(KeyedEvent[E], EventSource), Unit]): EventSubscription =
    new EventSourceEventSubscription[E](implicitClass[E], f)

  private class EventSourceEventSubscription[E <: Event](clas: Class[E], f: PartialFunction[(KeyedEvent[E], EventSource), Unit])
  extends EventSubscription {
    def eventClass = classOf[AnyKeyedEvent] // Its always KeyedEvent

    def handleEvent(keyedEvent: AnyKeyedEvent) = keyedEvent match {
      case KeyedEvent(key, EventSourceEvent(e: E @unchecked, source)) if clas isAssignableFrom e.getClass ⇒
        f.callIfDefined((KeyedEvent(e)(key.asInstanceOf[e.Key]), source))
      case _ ⇒
    }
  }
}
