package com.sos.scheduler.engine.common.async

final class CallRunner(val queue: PoppableCallQueue) {

  def execute(): Boolean = {
    val somethingDone = queue.isMature
    executeCalls()
    somethingDone
  }

  private def executeCalls() {
    while (true) {
      queue.popMature() match {
        case Some(o) => o.apply()
        case None => return
      }
    }
  }

  override def toString = s"${getClass.getSimpleName} with $queue"
}
