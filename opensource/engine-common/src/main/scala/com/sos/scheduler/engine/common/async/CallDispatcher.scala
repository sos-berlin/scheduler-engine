package com.sos.scheduler.engine.common.async

import javax.inject.Inject

final class CallDispatcher @Inject()(val queue: CallQueue) {

  def execute() {
    while (true) {
      queue.popMature() match {
        case Some(o) => o.apply()
        case None => return
      }
    }
  }

  override def toString = s"${getClass.getSimpleName} with $queue"
}
