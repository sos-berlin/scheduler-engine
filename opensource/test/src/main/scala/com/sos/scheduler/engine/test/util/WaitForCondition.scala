package com.sos.scheduler.engine.test.util

import java.lang.Thread.sleep
import org.joda.time.Instant.now
import org.joda.time.{ReadableInstant, Duration}

object WaitForCondition {

  /** Wartet längstens t.timeout in Schritten von t.step, bis condition wahr wird.
    * condition wird bei t.timeout > 0 wenigsten zweimal aufgerufen: am Anfang und am Ende.
    * @return letztes Ergebnis von condition */
  def waitForCondition(t: TimeoutWithSteps)(condition: => Boolean) =
    waitAtInstantsFor(t.toMillisInstantIterator(now()))(condition)

  /** Eine in Schritte unterteilte Frist.
    */
  final case class TimeoutWithSteps(timeout: Duration, step: Duration) {
    /** Liefert einen Iterator mit den Zeitpunkten startInstant, startInstant + step, ..., startInstant + timeout.
      */
    def toMillisInstantIterator(startInstant: ReadableInstant) =
      millisInstantIterator(startInstant.getMillis, timeout.getMillis, step.getMillis)
  }

  /** Liefert einen Iterator mit den Zeitpunkten startInstant, startInstant + step, ..., startInstant + timeout.
    */
  private[util] def millisInstantIterator(startInstant: Long, timeout: Long, step: Long): Iterator[Long] =
    ((0L to timeout - 1 by step).toIterator map { startInstant + _ }) ++ Iterator(startInstant + timeout)

  /** Wartet bis zu den Zeitpunkten, bis condition wahr wird.
    * condition wird am Anfang und am Ende geprüft.
    * @return letztes Ergebnis von condition */
  def waitAtInstantsFor(instants: TraversableOnce[Long])(condition: => Boolean) =
    realTimeIterator(instants) exists { _ => condition }

  /** Ein Iterator, der bei next() (oder hasNext) auf den nächsten Zeitpunkt wartet.
    * Wenn aufeinanderfolgende Zeitpunkte schon erreicht sind, kehrt der Iterator trotzdem jedesmal zurück.
    * Dass kann zu überflüssigen Aktionen des Aufrufers führen (aufeinanderfolgende Prüfung einer aufwändigen Bedingung). */
  def realTimeIterator(instants: TraversableOnce[Long]): Iterator[Unit] =
    instants.toIterator map sleepUntil    // toIterator führt dazu, das now() erst bei next() oder hasNext lazy aufgerufen wird.

  private def sleepUntil(until: Long) {
    val w = until - now().getMillis
    if (w > 0) sleep(w)
  }
}
