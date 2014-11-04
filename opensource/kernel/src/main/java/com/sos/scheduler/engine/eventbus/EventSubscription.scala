package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.data.event.Event
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: Event]
  def handleEvent(e: Event)

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
}
