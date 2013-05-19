package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.time.ScalaJoda._
import org.joda.time.Duration

final case class DatabaseConfiguration(use: Boolean, closeDelay: Duration = 0.s)
