package com.sos.scheduler.engine.common.async

import CallRunner._
import com.sos.scheduler.engine.common.scalautil.Logger

final class CallRunner(val queue: PoppableCallQueue) {

  def execute(n: Int = Int.MaxValue): Boolean = {
    val somethingDone = queue.isMature
    executeCalls(n)
    somethingDone
  }

  private def executeCalls(n: Int) {
    for (i <- 0 until n) {
      queue.popMature() match {
        case Some(o) => o.apply()
        case None => return
      }
    }
    if (n > 1)
      logger.debug(s"Interrupted after $n calls. next=${queue.matureHeadOption}")
  }

  override def toString = s"${getClass.getSimpleName} with $queue"
}

object CallRunner {
  private val logger = Logger(getClass)
}