package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.base.utils.ScalaUtils
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.data.event.Event
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: Event]
  def handleEvent(e: Event): Unit

  override def toString = s"EventSubscription(${eventClass.getSimpleName})"
}


object EventSubscription {

  def apply[E <: Event : ClassTag](f: E ⇒ Unit): EventSubscription =
    new Impl(implicitClass[E], f.asInstanceOf[Event ⇒ Unit])

  final class Impl[E <: Event](clas: Class[E], f: E ⇒ Unit) extends EventSubscription {
    def eventClass = clas

    def handleEvent(e: Event) = {
      val unwrappedEvent = e match {
        case EventSourceEvent(event, _) ⇒ event
        case _ ⇒ e
      }
      f(unwrappedEvent.asInstanceOf[E])
    }
  }

  def withSource[E <: Event: ClassTag](f: PartialFunction[(E, EventSource), Unit]): EventSubscription =
    new EventSourceEventSubscription[E](implicitClass[E], f)

  private class EventSourceEventSubscription[E <: Event](clas: Class[E], f: PartialFunction[(E, EventSource), Unit]) extends EventSubscription {
    def eventClass = clas

    def handleEvent(e: Event) = e match {
      case EventSourceEvent(e: E @unchecked, source) if eventClass isAssignableFrom e.getClass ⇒
        f.applyOrElse((e, source), identity[(E, EventSource)])
      case _ ⇒
    }
  }
}
