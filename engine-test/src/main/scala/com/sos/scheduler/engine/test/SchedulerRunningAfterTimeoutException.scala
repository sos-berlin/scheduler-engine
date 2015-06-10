package com.sos.scheduler.engine.test

import java.time.Duration
import com.sos.scheduler.engine.common.time.ScalaTime._

class SchedulerRunningAfterTimeoutException(timeout: Duration)
extends RuntimeException(s"Scheduler has not been terminated within ${timeout.pretty}")
