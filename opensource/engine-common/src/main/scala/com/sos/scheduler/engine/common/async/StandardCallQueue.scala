package com.sos.scheduler.engine.common.async

import java.lang.System.currentTimeMillis
import scala.collection.mutable

final class StandardCallQueue extends PoppableCallQueue {
  private val queue = mutable.UnrolledBuffer[TimedCall[_]]()

  def add(o: TimedCall[_]) {
    synchronized {
      val i = positionAfter(o.at)
      if (i < queue.size) queue.insert(i, o)
      else queue.append(o)  // Scala 2.10.0: queue.insert(queue.size, x) geht in eine Schleife
    }
  }

  def tryRemove(o: TimedCall[_]): Boolean = {
    synchronized {
      indexOf(o) match {
        case -1 => false
        case i => queue.remove(i); true
      }
    }
  }

  def isEmpty = synchronized { queue.isEmpty }

  private def indexOf(o: TimedCall[_]) =
    queue indexWhere { _ eq o }

  private def positionAfter(at: Long) = queue indexWhere { _.at > at } match {
    case -1 => queue.size
    case i => i
  }

  def popMature(): Option[TimedCall[_]] = synchronized {
    queue.headOption collect { case o if o.at <= currentTimeMillis() => queue.remove(0) ensuring { _ == o } }
  }

  override def toString = s"${getClass.getSimpleName} with ${queue.size} operations, next=${queue.headOption}"
}
