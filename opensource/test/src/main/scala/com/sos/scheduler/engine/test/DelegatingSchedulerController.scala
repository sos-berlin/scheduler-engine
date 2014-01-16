package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.time.Time
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.main.SchedulerController

trait DelegatingSchedulerController extends SchedulerController {

  protected val delegate: SchedulerController

  final def terminateScheduler() {
    delegate.terminateScheduler()
  }

  final def terminateAfterException(x: Throwable) {
    delegate.terminateAfterException(x)
  }

  final def tryWaitForTermination(timeout: Time) =
    delegate.tryWaitForTermination(timeout)

  final def exitCode =
    delegate.exitCode

  final def getEventBus =
    delegate.getEventBus
}

