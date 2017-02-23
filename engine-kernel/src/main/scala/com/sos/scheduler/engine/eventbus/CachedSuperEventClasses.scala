package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.data.event.Event
import com.sos.scheduler.engine.eventbus.CachedSuperEventClasses._
import scala.collection.mutable

/**
  * @author Joacim Zschimmer
  */
final class CachedSuperEventClasses {
  private val store = new java.util.concurrent.ConcurrentHashMap[Class[_ <: Event], Set[Class[_ <: Event]]]

  def apply(eventClass: Class[_ <: Event]): Set[Class[_ <: Event]] =
    store.get(eventClass) match {
      case null ⇒
        val classes = superEventClassesOf(eventClass)
        store.put(eventClass, classes)
        classes
      case o ⇒ o
    }
}

object CachedSuperEventClasses {
  private def superEventClassesOf(clas: Class[_]): Set[Class[_ <: Event]] = {
    val classes = mutable.Set[Class[_ <: Event]]()
    collectAllSuperEventClasses(clas, classes)
    classes.toSet
  }

  private def collectAllSuperEventClasses(clas: Class[_], result: mutable.Set[Class[_ <: Event]]): Unit = {
    if (clas != null && classOf[Event].isAssignableFrom(clas)) {
      val eventClass = clas.asInstanceOf[Class[_ <: Event]]
      if (!result.contains(eventClass)) {
        result += eventClass
        collectAllSuperEventClasses(clas.getSuperclass, result)
        for (c ← clas.getInterfaces) collectAllSuperEventClasses(c, result)
      }
    }
  }
}
