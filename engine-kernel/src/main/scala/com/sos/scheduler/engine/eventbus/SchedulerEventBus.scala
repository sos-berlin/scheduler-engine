package com.sos.scheduler.engine.eventbus

import com.google.common.io.Closer
import com.sos.scheduler.engine.base.utils.ScalaUtils.{RichUnitPartialFunction, implicitClass}
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent}
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
  def onHotAndCold[E <: Event](eventHandler: PartialFunction[KeyedEvent[E], Unit])(implicit closer: Closer, e: ClassTag[E]): Unit = {
    onHot[E](eventHandler)(closer, e)
    on[E](eventHandler)(closer, e)
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
  def onHot[E <: Event](eventHandler: PartialFunction[KeyedEvent[E], Unit])(implicit closer: Closer, e: ClassTag[E]): Unit =
    subscribeKeyedClosable[E](hotEventBus, eventHandler)

  /**
   * Subscribes events of class [[EventSourceEvent]][E] for `eventHandler` and registers this in [[Closer]] for automatic unsubscription with Closer.closer().
   * The event handler is synchronous.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHotEventSourceEvent[E <: Event: ClassTag](eventHandler: PartialFunction[KeyedEvent[EventSourceEvent[E]], Unit])
    (implicit closer: Closer, e: ClassTag[E]): Unit
  = {
    val subscription = new EventSubscription {
      val eventClass = classOf[AnyKeyedEvent]
      def handleEvent(e: AnyKeyedEvent) =
        e match {
          case keyedEvent @ KeyedEvent(_, e: EventSourceEvent[E @unchecked]) ⇒
            if (implicitClass[E] isAssignableFrom e.event.getClass) {
              eventHandler.callIfDefined(keyedEvent.asInstanceOf[KeyedEvent[EventSourceEvent[E]]])
            }
          case _ ⇒
        }
    }
    subscribeClosable[AnyKeyedEvent](hotEventBus, subscription)
  }

  private def subscribeKeyedClosable[E <: Event: ClassTag](whichEventBus: EventBus, handleEvent: PartialFunction[KeyedEvent[E], Unit])
    (implicit closer: Closer, e: ClassTag[E]): Unit
  =
    subscribeClosable[AnyKeyedEvent](whichEventBus, EventSubscription[KeyedEvent[E]] {
      case e: KeyedEvent[E @unchecked] if implicitClass[E] isAssignableFrom e.event.getClass ⇒
        handleEvent.callIfDefined(e)
    })

  private def subscribeClosable[E <: AnyKeyedEvent](whichEventBus: EventBus, subscription: EventSubscription)(implicit closer: Closer, e: ClassTag[E]): Unit = {
    whichEventBus.register(subscription)
    closer.onClose {
      whichEventBus.unregister(subscription)
    }
  }
  def register(s: EventSubscription): Unit = coldEventBus.register(s)

  def unregister(s: EventSubscription): Unit = coldEventBus.unregister(s)

  def registerHot(s: EventSubscription): Unit = hotEventBus.register(s)

  def unregisterHot(s: EventSubscription): Unit = hotEventBus.unregister(s)

  def isSubscribed = hotEventBus.isSubscribed || coldEventBus.isSubscribed

  def publish[E <: AnyKeyedEvent](e: E, source: EventSource): Unit = {
    logger.trace(s"Publish $e")
    hotEventBus.publish(KeyedEvent(new EventSourceEvent(e.event, source))(e.key))
    coldEventBus.publish(e)
  }

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
