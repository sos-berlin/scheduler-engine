package com.sos.scheduler.engine.test

import org.joda.time.Duration
import com.sos.scheduler.engine.common.time.ScalaJoda._

class SchedulerRunningAfterTimeoutException(timeout: Duration)
extends RuntimeException(s"Scheduler has not been terminated within ${timeout.pretty}")
