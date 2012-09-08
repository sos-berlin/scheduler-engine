package com.sos.scheduler.engine.test.util

import java.lang.Thread.sleep
import org.joda.time.Instant.now
import org.joda.time.{ReadableInstant, Duration}

object WaitForCondition {

  /** Wartet längstens die Dauer d in Schritten von step, bis condition wahr wird.
    * condition wird bei d > 0 wenigsten zweimal aufgerufen: am Anfang und am Ende.
    * @return letztes Ergebnis von condition */
  def waitForCondition(t: TimeoutWithSteps)(condition: => Boolean) =
    waitAtInstantsFor(t.toMillisInstantIterator(now()))(condition)

  /** Wartet bis zu den Zeitpunkten, bis condition wahr wird.
    * condition wird am Anfang und am Ende aufgerufen.
    * @return letztes Ergebnis von condition */
  def waitAtInstantsFor(instances: TraversableOnce[Long])(condition: => Boolean) =
    condition || (millisFromNowUntilInstanceIterator(instances) exists { w => sleep(w); condition })

  def millisFromNowUntilInstanceIterator(instances: TraversableOnce[Long]) =
    instances.toIterator map { _ - now().getMillis } filter { _ > 0 }     // toIterator führt dazu, das now() lazy erst bei next() oder hasNext aufgerufen wird.

  final case class TimeoutWithSteps(timeout: Duration, step: Duration) {
    def toMillisInstantIterator(startInstant: ReadableInstant) =
      millisInstantIterator(startInstant.getMillis, timeout.getMillis, step.getMillis)
  }

  private[util] def millisInstantIterator(startInstant: Long, timeout: Long, step: Long): Iterator[Long] =
    ((step to timeout - 1 by step).toIterator map { o => startInstant + o }) ++ Iterator(startInstant + timeout)
}
