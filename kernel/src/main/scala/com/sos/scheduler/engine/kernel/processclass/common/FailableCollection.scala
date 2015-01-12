package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.SingleThreaded
import com.sos.scheduler.engine.kernel.processclass.common.EndlessRepeatedIterator.endlessRepeatedIterator
import org.joda.time.{Duration, Instant}
import org.scalactic.Requirements._
import scala.collection.{immutable, mutable}

/**
 * Verwaltet eine Sequenz von Failables, die reihum geliefert werden.
 * Failables können versagen und werden dann failureTimeout lang nicht geliefert.
 * @param failureTimeout A function because C++ Settings is not yet freezed (=readable) when &process_class is defined in scheduler.xml.
 * @author Joacim Zschimmer
 */
class FailableCollection[Failable](failables: immutable.Seq[Failable], failureTimeout: () ⇒ Duration)
extends SingleThreaded {

  require(failables.nonEmpty)
  require(failables.distinct.size == failables.size)

  private case class TimestampedFailure(instant: Instant, throwable: Throwable) {
    def isDelayed = delay > 0.s
    def delay = instant + failureTimeout() - now
  }

  private val failureMap = mutable.LinkedHashMap[Failable, TimestampedFailure]()
  private val iterator = endlessRepeatedIterator { failables.iterator } .buffered

  /** Liefert eine Verzögerung, wenn alle Entitäten innerhalb des letzten failureTimeout einen Fehler hatten.
    * @return (0.s, next A, which has not failed within failureTimeout) or (minimumDelay, next failable) */
  final def nextDelayAndEntity(): (Duration, Failable) = {
    requireMyThread()
    iterator take failables.size find { o ⇒ !isDelayed(o) } match {
      case Some(failable) ⇒ 0.s → failable
      case None ⇒ oldestTimestampedFailure match { case (failable, t) ⇒ (t.delay, failable) }
    }
  }

  private def oldestTimestampedFailure: (Failable, TimestampedFailure) = failureMap minBy { _._2.instant }

  /** Only needed to clear a failure before failureTimeout. */
  final def clearFailure(failable: Failable): Unit = {
    requireMyThread()
    failureMap -= failable
  }

  final def setFailure(failable: Failable, throwable: Throwable): Unit = {
    requireMyThread()
    failureMap += failable → TimestampedFailure(now, throwable)
  }

  private def isDelayed(o: Failable) = failureMap.get(o) exists { _.isDelayed }

  protected def now: Instant = Instant.now
}
