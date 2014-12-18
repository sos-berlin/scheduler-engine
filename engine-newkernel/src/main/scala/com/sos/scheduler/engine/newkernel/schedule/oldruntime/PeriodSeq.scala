package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import PeriodSeq._
import com.sos.scheduler.engine.newkernel.schedule.TimeOfDay
import scala.collection.immutable

final class PeriodSeq(val orderedSeq: immutable.Seq[OldPeriod]) {
  requireOrderedAndNotOverlapping(orderedSeq)

//  def nextInstant(from: DateTime): Option[Instant] = {
//    val date = from.toDateMidnight
//    val timeOfDay = TimeOfDay(from.getMillisOfDay)
//    val result = nextTimeOfDay(timeOfDay)
//    result map { o => new Instant(date.getMillis + o.millis) }
//  }

  def nextTimeOfDay(from: TimeOfDay): Option[TimeOfDay] =
    (orderedSeq flatMap { _.startNotBefore(from) }).headOption

//  def nextInterval(from: DateTime): Option[Interval] = {
//    val date = from.toDateMidnight
//    val timeOfDay = TimeOfDay(from.getMillisOfDay)
//    nextPeriod(timeOfDay)  map periodToInterval(date)
//  }

  def nextPeriod(from: TimeOfDay): Option[OldPeriod] =
    orderedSeq find { _ contains from }
}

object PeriodSeq {
  def apply(o: Seq[OldPeriod]) =
    new PeriodSeq((o sortWith { _.begin < _.begin }).to[immutable.Seq])

  private def requireOrderedAndNotOverlapping(orderedPeriods: Iterable[OldPeriod]): Unit = {
    if (orderedPeriods.size > 2) {
      for (Seq(a, b) <- orderedPeriods sliding 2) {
        require(a.end <= b.begin)
      }
    }
  }
}
