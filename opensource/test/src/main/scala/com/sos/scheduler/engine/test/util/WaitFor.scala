package com.sos.scheduler.engine.test.util

import org.joda.time.{Duration, DateTime}
import org.joda.time.DateTime.now
import java.lang.Thread.sleep


object WaitFor {

  /** Wartet längstens die Dauer d in Schritten von step, bis condition wahr wird.
    * condition wird bei d > 0 wenigsten zweimal aufgerufen: am Anfang und am Ende.
    * @return letztes Ergebnis von condition */
  def waitFor(d: Duration, step: Duration)(condition: => Boolean): Boolean =
    waitFor(durationIterator(d, step))(condition)

  /** Liefert einen Iterator beginnend mit step, dann in Schritten von step, zum Schluss d. **/
  def durationIterator(d: Duration, step: Duration) =
    ((step.getMillis until d.getMillis by step.getMillis) ++ Seq(d)) map { o => new Duration(o) }

  /** Wartet die waitPeriods, bis condition wahr wird.
    * condition wird am Anfang und am Ende aufgerufen.
    * @return letztes Ergebnis von condition */
  def waitFor(waitPeriods: TraversableOnce[Duration])(condition: => Boolean): Boolean =
    waitForAtPoints(pointIterator(now(), waitPeriods))(condition)

  def pointIterator(t: DateTime, waitPeriods: TraversableOnce[Duration]) = waitPeriods.toIterator map { o => t plus o }

  /** Wartet bis zu den Zeitpunkten, bis condition wahr wird.
    * condition wird am Anfang und am Ende aufgerufen.
    * @return letztes Ergebnis von condition */
  def waitForAtPoints(points: TraversableOnce[DateTime])(condition: => Boolean): Boolean =
    condition || {
      val i = waitTimeNowIterator(points)
      i exists { w => i.hasNext && { sleep(i.next().getMillis); condition } }
    }

  def waitTimeNowIterator(i: TraversableOnce[DateTime]): Iterator[Duration] =
    i.toIterator map { o => new Duration(DateTime.now(), o) } filter { _ isLongerThan Duration.ZERO }
}
