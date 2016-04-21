package com.sos.scheduler.engine.eventbus

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.data.event.Event
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
   * Calls methods `onHot` and `on`.
   * `eventHandler` will be called twice: synchronously at the event and later asynchronously.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHotAndCold[E <: Event](eventHandler: PartialFunction[E, Unit])(implicit closer: Closer, e: ClassTag[E]): Unit = {
    onHot[E](eventHandler)(closer, e)
    on[E](eventHandler)(closer, e)
  }

  /**
   * Subscribes events of class E for eventHandler and registers this in [[Closer]] for unsubscription with Closer.closer().
   * The event handler is called asynchronous by dispatchEvent().
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def on[E <: Event](eventHandler: PartialFunction[E, Unit])(implicit closer: Closer, e: ClassTag[E]): Unit =
    subscribeClosable[E](coldEventBus, eventHandler)

  /**
   * Subscribes events of class E for eventHandler and registers this in [[Closer]] for unsubscription with Closer.closer().
   * The event handler is synchronous.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHot[E <: Event](eventHandler: PartialFunction[E, Unit])(implicit closer: Closer, e: ClassTag[E]): Unit =
    subscribeClosable[E](hotEventBus, eventHandler)

  /**
   * Subscribes events of class [[EventSourceEvent]][E] for `eventHandler` and registers this in [[Closer]] for automatic unsubscription with Closer.closer().
   * The event handler is synchronous.
   * @param eventHandler Events not in the function's domain are ignored.
   */
  def onHotEventSourceEvent[E <: Event](eventHandler: PartialFunction[EventSourceEvent[E], Unit])(implicit closer: Closer, e: ClassTag[E]): Unit = {
    val subscription = new EventSubscription {
      val eventClass = implicitClass[E]
      def handleEvent(e: Event) = eventHandler.applyOrElse(e.asInstanceOf[EventSourceEvent[E]], (_: EventSourceEvent[E]) ⇒ ())
    }
    subscribeClosable[E](hotEventBus, subscription)
  }

  private def subscribeClosable[E <: Event](whichEventBus: EventBus, eventHandler: PartialFunction[E, Unit])(implicit closer: Closer, e: ClassTag[E]): Unit = {
    val subscription = new EventSubscription.Impl(implicitClass[E], eventHandler orElse PartialFunction { _: E ⇒ })
    subscribeClosable[E](whichEventBus, subscription)
  }

  private def subscribeClosable[E <: Event](whichEventBus: EventBus, subscription: EventSubscription)(implicit closer: Closer, e: ClassTag[E]): Unit = {
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

  def publish[E <: Event](e: E, source: EventSource): Unit = publish(new EventSourceEvent(e, source))

  def publish(e: Event): Unit = {
    logger.trace(s"Publish $e")
    hotEventBus.publish(e)
    publishCold(e)
  }

  def publishCold(event: Event): Unit =
    coldEventBus.publish(event match {
      case o: EventSourceEvent[_] ⇒ o.event
      case _ ⇒ event
    })

  def dispatchEvents(): Unit = coldEventBus.dispatchEvents()

  def run(): Unit = coldEventBus.run()

  override def toString = getClass.getSimpleName
}

object SchedulerEventBus {
  private final val logger = Logger(getClass)
}
