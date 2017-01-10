package com.sos.scheduler.engine.eventbus

import com.google.common.io.Closer
import com.sos.scheduler.engine.base.utils.ScalaUtils.RichUnitPartialFunction
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent, NoKeyEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus._
import scala.reflect.ClassTag

final class SchedulerEventBus extends EventBus with Runnable {

  private val hotEventBus = new HotEventBus
  private val coldEventBus = new ColdEventBus

  def registerAnnotated(o: EventHandlerAnnotated): Unit = {
    hotEventBus.registerAnnotated(o)
    coldEventBus.registerAnnotated(o)
  }

  def unregisterAnnotated(o: EventHandlerAnnotated): Unit = {
    hotEventBus.unregisterAnnotated(o)
    coldEventBus.unregisterAnnotated(o)
  }

  /**
   * Calls methods `onHotBasic` and `on`.
   * `eventHandler` will be called twice: synchronously at the event and later asynchronously.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHotAndCold[E <: Event: ClassTag](eventHandler: PartialFunction[KeyedEvent[E], Unit])(implicit closer: Closer): Unit = {
    onHot[E](eventHandler)
    on[E](eventHandler)
  }

  /**
   * Subscribes events of class E for eventHandler and registers this in [[Closer]] for unsubscription with Closer.closer().
   * The event handler is called asynchronous by dispatchEvent().
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def on[E <: Event](eventHandler: PartialFunction[KeyedEvent[E], Unit])(implicit closer: Closer, e: ClassTag[E]): Unit =
    subscribeKeyedClosable[E](coldEventBus, eventHandler)

  /**
   * Subscribes events of class E for eventHandler and registers this in [[Closer]] for unsubscription with Closer.closer().
   * The event handler is synchronous.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHot[E <: Event: ClassTag](eventHandler: PartialFunction[KeyedEvent[E], Unit])(implicit closer: Closer): Unit =
    subscribeKeyedClosable[E](hotEventBus, eventHandler)

  private def subscribeKeyedClosable[E <: Event: ClassTag](whichEventBus: EventBus, handleEvent: PartialFunction[KeyedEvent[E], Unit])
    (implicit closer: Closer): Unit
  =
    subscribeClosable[E](whichEventBus, EventSubscription[E] {
      case e ⇒ handleEvent.callIfDefined(e)
    })

  private def subscribeClosable[E <: Event: ClassTag](whichEventBus: EventBus, subscription: EventSubscription)(implicit closer: Closer): Unit = {
    whichEventBus.subscribe(subscription)
    closer.onClose {
      whichEventBus.unsubscribe(subscription)
    }
  }
  def subscribe(s: EventSubscription): Unit = coldEventBus.subscribe(s)

  def unsubscribe(s: EventSubscription): Unit = coldEventBus.unsubscribe(s)

  def subscribeHot(s: EventSubscription): Unit = hotEventBus.subscribe(s)

  def unsubscribeHot(s: EventSubscription): Unit = hotEventBus.unsubscribe(s)

  def isSubscribed = hotEventBus.isSubscribed || coldEventBus.isSubscribed

  def publish(e: NoKeyEvent): Unit = publish(KeyedEvent(e))

  def publish(e: AnyKeyedEvent): Unit = {
    logger.trace(s"Publish $e")
    hotEventBus.publish(e)
    publishCold(e)
  }

  def publishCold(keyedEvent: AnyKeyedEvent): Unit =
    coldEventBus.publish(keyedEvent)

  def dispatchEvents(): Unit = coldEventBus.dispatchEvents()

  def run(): Unit = coldEventBus.run()

  override def toString = getClass.getSimpleName
}

object SchedulerEventBus {
  private final val logger = Logger(getClass)
}
