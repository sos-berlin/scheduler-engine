package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.base.utils.ScalaUtils.{RichUnitPartialFunction, implicitClass}
import com.sos.jobscheduler.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import scala.reflect.ClassTag

trait EventSubscription {
  def eventClass: Class[_ <: Event]
  def handleEvent(e: AnyKeyedEvent): Unit

  override def toString = s"EventSubscription(${eventClass.getSimpleName})"
}

object EventSubscription {

  def apply[E <: Event: ClassTag](handleEvent: PartialFunction[KeyedEvent[E], Unit]): EventSubscription =
    forClass(implicitClass[E])(handleEvent)

  def forClass[E <: Event](clas: Class[E])(handleEvent: PartialFunction[KeyedEvent[E], Unit]): EventSubscription =
    new Standard(clas, handleEvent)

  private class Standard[E <: Event](clas: Class[E], handle: PartialFunction[KeyedEvent[E], Unit]) extends EventSubscription {
    def eventClass = clas

    def handleEvent(e: AnyKeyedEvent) = handle.callIfDefined(e.asInstanceOf[KeyedEvent[E]])
  }
}
