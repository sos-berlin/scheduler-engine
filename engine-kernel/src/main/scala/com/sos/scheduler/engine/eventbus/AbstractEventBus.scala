package com.sos.scheduler.engine.eventbus

import com.google.common.collect.{HashMultimap, Multimap}
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.AbstractEventBus.{logger, matches}
import com.sos.scheduler.engine.eventbus.annotated.MethodEventSubscriptionFactory
import scala.collection.JavaConversions._
import scala.collection.immutable.VectorBuilder

trait AbstractEventBus extends EventBus {
  protected def factory: MethodEventSubscriptionFactory

  final private val subscriptionRegister: Multimap[Class[_ <: Event], EventSubscription] = HashMultimap.create()
  final private val handlerFinder = new AnnotatedHandlerFinder(factory)
  final private val annotatedEventSubscriberMap: Multimap[EventHandlerAnnotated, EventSubscription] = HashMultimap.create()
  private final val superEventClasses = new CachedSuperEventClasses

  final def registerAnnotated(o: EventHandlerAnnotated): Unit = {
    unregisterAnnotated(o)
    val subscribers = handlerFinder.handlers(o)
    annotatedEventSubscriberMap.putAll(o, subscribers)
    registerAll(subscribers)
  }

  final def unregisterAnnotated(o: EventHandlerAnnotated): Unit = {
    unregisterAll(annotatedEventSubscriberMap.get(o))
    annotatedEventSubscriberMap.removeAll(o)
  }

  private def registerAll(subscribers: Iterable[EventSubscription]): Unit = {
    for (s ← subscribers) subscribe(s)
  }

  private def unregisterAll(subscribers: Iterable[EventSubscription]): Unit = {
    for (s ← subscribers) unsubscribe(s)
  }

  final def subscribe(s: EventSubscription): Unit = {
    synchronized {
      subscriptionRegister.put(s.eventClass, s)
    }
  }

  final def unsubscribe(s: EventSubscription): Unit = {
    val ok = synchronized {
      subscriptionRegister.remove(s.eventClass, s)
    }
    if (!ok) logger.debug(s"unsubscribe unknown '$s'")
  }

  final def isSubscribed: Boolean = !synchronized { subscriptionRegister.isEmpty }

  final protected def calls(e: AnyKeyedEvent): Vector[Call] = {
    var result: VectorBuilder[Call] = null
    val classes = superEventClasses(e.event.getClass)
    synchronized {
      for (c ← classes;
           s ← subscriptionRegister.get(c) if matches(s, e)) {
        if (result == null) result = new VectorBuilder[Call]()
        result += Call(e, s)
      }
    }
    if (result != null) result.result else Vector()
  }

  final protected def dispatchCall(call: Call): Boolean =
    try {
      call.apply()
      true
    }
    catch {
      case t: Throwable ⇒  // Der C++-Code soll wirklich keine Exception bekommen.
        if (t.isInstanceOf[Error] && t.getClass.getSimpleName == "TestError")
          logger.debug(s"$call", t)
        else
          logger.error(s"$call", t)
        call.keyedEvent.event match {
          case _: EventHandlerFailedEvent ⇒ false
          case _ ⇒ publishNow(KeyedEvent(new EventHandlerFailedEvent(call, t)))
        }
    }

  private def publishNow(e: AnyKeyedEvent): Boolean = {
    var published = false
    for (call ← calls(e)) {
      dispatchCall(call)
      published = true
    }
    published
  }

}

object AbstractEventBus {
  private val logger = Logger(getClass)

  private def matches(s: EventSubscription, e: AnyKeyedEvent): Boolean = {
    true // KeyEvent is the only event class
    //        return !(s instanceof EventSourceMethodEventSubscription) ||
    //                e instanceof EventSourceEvent &&
    //                        ((EventSourceMethodEventSubscription)s).eventSourceMatches((EventSourceEvent<?>)e);
  }
}
