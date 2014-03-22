package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.data.scheduler.SchedulerId
import org.joda.time.Instant

final case class SchedulerOverview(
  version: String,
  commitNumber: String,
  startInstant: Instant,
  instant: Instant,
  schedulerId: SchedulerId,
  tcpPort: Option[Int],
  udpPort: Option[Int],
  state: String)
