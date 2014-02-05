package com.sos.scheduler.engine.newkernel.schedule

import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.newkernel.utils.TimedCallHolder
import org.joda.time.Instant._
import org.joda.time.Interval

final class IntervalSelector(schedule: Schedule, callQueue: CallQueue) extends AutoCloseable {
  private var currentRunnableInterval: Option[Interval] = None
  private var intervalStartCallHolder = new TimedCallHolder(callQueue)

  def close() {
    intervalStartCallHolder.close()
  }

  def start() {
    val n = now()
    schedule.nextInterval(n) match {
      case None =>
        setCurrentRunnableInterval(None)
      case Some(interval) =>
        if (n isBefore interval.getStart)
          intervalStartCallHolder.enqueue(interval.getStart.toInstant) {
            if (now() isBefore interval.getEnd)
              setCurrentRunnableInterval(Some(interval))
            else
              start()      // Wir sind zu spät. Also nehmen wir das folgende Intervall
          }
        else
          currentRunnableInterval = Some(interval)
    }
  }

  private def setCurrentRunnableInterval(intervalOption: Option[Interval]) {
    intervalOption match {
      case Some(interval) =>
        currentRunnableInterval = Some(interval)
        intervalStartCallHolder.enqueue(interval.getEnd.toInstant) { start() }
      case None =>
        currentRunnableInterval = None
        intervalStartCallHolder.cancel()
    }
  }
}
