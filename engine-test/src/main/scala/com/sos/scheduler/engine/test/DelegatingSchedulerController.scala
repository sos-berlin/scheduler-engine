package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.main.SchedulerController
import org.joda.time.Duration

trait DelegatingSchedulerController extends SchedulerController {

  protected val delegate: SchedulerController

  final def terminateScheduler(): Unit = {
    delegate.terminateScheduler()
  }

  final def terminateAfterException(x: Throwable): Unit = {
    delegate.terminateAfterException(x)
  }

  final def tryWaitForTermination(timeout: Duration) =
    delegate.tryWaitForTermination(timeout)

  final def exitCode =
    delegate.exitCode

  final def getEventBus =
    delegate.getEventBus
}

