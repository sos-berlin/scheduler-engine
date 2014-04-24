package com.sos.scheduler.engine.common.async

import StandardCallQueue._
import com.sos.scheduler.engine.common.scalautil.Logger
import org.joda.time.DateTimeUtils.currentTimeMillis
import scala.collection.mutable
import scala.util.control.NonFatal

final class StandardCallQueue extends PoppableCallQueue {
  private val queue = mutable.Buffer[TimedCall[_]]()
//private val queue = mutable.UnrolledBuffer[TimedCall[_]]()    Scala 2.10.0 insert() terminiert nicht
  private var closed = false

  def add(o: TimedCall[_]) {
    logger debug s"Enqueue at ${o.atString} $o"
    synchronized {
      if (closed)  sys.error(s"CallQueue is closed. '$o' is rejected")
      val i = positionAfter(o.epochMillis)
      if (i < queue.size) queue.insert(i, o)
      else queue.append(o)  // Scala 2.10.0: queue.insert(queue.size, x) geht in eine Schleife
    }
  }

  def close() {
    val copy = synchronized {
      closed = true
      val result = queue.toVector
      queue.clear()
      result
    }
    for (o <- copy) {
      try o.onCancel()
      catch { case NonFatal(e) ⇒ logger.warn(s"$o.onCancel(): $e", e) }
    }
  }

  def tryCancel(o: TimedCall[_]): Boolean = {
    val cancelled = synchronized {
      indexOf(o) match {
        case -1 ⇒ None
        case i ⇒ Some(queue.remove(i))
      }
    }
    for (o <- cancelled) o.onCancel()
    cancelled.isDefined
  }

  def isEmpty =
    synchronized { queue.isEmpty }

  private def indexOf(o: TimedCall[_]) =
    queue indexWhere { _ eq o }

  private def positionAfter(at: Long) =
    queue indexWhere { _.epochMillis > at } match {
      case -1 => queue.size
      case i => i
    }

  def nextTime =
    headOption map { _.epochMillis } getOrElse Long.MaxValue

  def isMature =
    matureHeadOption.nonEmpty

  def popMature(): Option[TimedCall[_]] =
    synchronized {
      matureHeadOption map { o => queue.remove(0) ensuring { _ == o } }
    }

  def matureHeadOption =
    headOption filter timedCallIsMature

  private def headOption =
    synchronized { queue.headOption }

  private def timedCallIsMature(o: TimedCall[_]) =
    o.epochMillis <= currentTimeMillis()

  override def toString =
    s"${getClass.getSimpleName} with ${queue.size} operations, next=${queue.headOption}"
}

object StandardCallQueue {
  private val logger = Logger(getClass)
}
