package com.sos.scheduler.engine.common.async

import CallRunner._
import com.sos.scheduler.engine.common.scalautil.Logger
import scala.annotation.tailrec

final class CallRunner(val queue: PoppableCallQueue) {

  def execute(n: Int = Int.MaxValue): Boolean = {
    val somethingDone = queue.isMature
    executeCalls(n)
    somethingDone
  }

  @tailrec private def executeCalls(n: Int) {
    if (n == 0) {
      queue.matureHeadOption foreach { o => logger debug s"Interrupted after $n calls. next=$o" }
    } else {
      queue.popMature() match {
        case Some(o) =>
          o.apply()
          executeCalls(n - 1)
        case None =>
      }
    }
  }

  override def toString = s"${getClass.getSimpleName} with $queue"
}

object CallRunner {
  private val logger = Logger(getClass)
}