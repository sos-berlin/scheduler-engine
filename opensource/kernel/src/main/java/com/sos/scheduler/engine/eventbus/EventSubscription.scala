package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.data.event.Event
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: Event]
  def handleEvent(e: Event)
}


object EventSubscription {

  def apply[E <: Event](f: E => Unit)(implicit e: ClassTag[E]) =
    new EventSubscription {
      def eventClass =e.runtimeClass.asInstanceOf[Class[E]]
      def handleEvent(e: Event) = f(e.asInstanceOf[E])
    }
}
