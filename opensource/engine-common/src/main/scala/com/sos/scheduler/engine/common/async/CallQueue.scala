package com.sos.scheduler.engine.common.async

import scala.collection.mutable
import scala.sys.error

final class CallQueue {
  protected val queue = mutable.UnrolledBuffer[TimedCall[_]]()

  def add[A](f: => A) {
    add(ShortTermCall[A](f))
  }

  def add(o: TimedCall[_]) {
    synchronized {
      val i = positionAfter(o.at)
      if (i < queue.size) queue.insert(i, o)
      else queue.append(o)  // Scala 2.10.0: queue.insert(queue.size, x) geht in eine Schleife
    }
  }

  def remove(o: TimedCall[_]) {
    val removed = tryRemove(o)
    if (!removed) error(s"Unknown TimedCall '$o'")
  }

  def tryRemove(o: TimedCall[_]): Boolean = {
    synchronized {
      indexOf(o) match {
        case -1 => false
        case i => queue.remove(i); true
      }
    }
  }

  def nonEmpty = !isEmpty

  def isEmpty = synchronized { queue.isEmpty }

  def popMature(): Option[TimedCall[_]] = synchronized {
    queue.headOption collect { case o if o.at <= System.currentTimeMillis() => queue.remove(0) ensuring { _ == o } }
  }

  private def indexOf(o: TimedCall[_]) =
    queue indexWhere { _ eq o }

  private def positionAfter(at: Long) = queue indexWhere { _.at > at } match {
    case -1 => queue.size
    case i => i
  }

  override def toString = s"${getClass.getSimpleName} with ${queue.size} operations, next=${queue.headOption}"
}
