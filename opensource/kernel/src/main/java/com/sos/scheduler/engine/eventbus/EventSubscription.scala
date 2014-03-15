package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.data.event.Event
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: Event]
  def handleEvent(e: Event)
}


object EventSubscription {

  def apply[E <: Event : ClassTag](f: E => Unit): EventSubscription =
    new EventSubscription {
      def eventClass = implicitClass[E]

      def handleEvent(e: Event) = {
        val realEvent = e match {
          case e: EventSourceEvent => e.getEvent
          case _ => e
        }
        f(realEvent.asInstanceOf[E])
      }
    }
}
