package com.sos.scheduler.engine.newkernel.schedule

import scala.collection.immutable
import scala.sys.error

abstract sealed class Weekday(val number: Int, val name: String) {
  override def hashCode = number.hashCode

  override def equals(o: Any) = o match {
    case q: Weekday => number == q.number
    case _ => false
  }
}

object Weekday {
  object Monday extends Weekday(1, "monday")
  object Tuesday extends Weekday(2, "tuesday")
  object Wednesday extends Weekday(3, "wednesday")
  object Thursday extends Weekday(4, "thursday")
  object Friday extends Weekday(5, "friday")
  object Saturday extends Weekday(6, "saturday")
  object Sunday extends Weekday(7, "sunday")

  private val weekdays = immutable.Seq(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)
  private val stringWeekdayMap = ((weekdays map { o => o.name -> o }) ++ (weekdays map { o => o.name.substring(0, 3) -> o })).toMap
  private val numberWeekdayMap = (weekdays map { o => o.number-> o }).toMap

  def apply(s: String): Weekday =
    stringWeekdayMap.getOrElse(s, numberWeekdayMap.getOrElse(normalizeZeroSunday(s.toInt), error(s"Not a weekday: $s")))

  private def normalizeZeroSunday(o: Int) = if (o == 0) 7 else o

  def apply(i: Int): Weekday =
    numberWeekdayMap.getOrElse(i, error(s"Not a weekday number between 1 and 7: $i"))
}
