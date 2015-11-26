package com.sos.scheduler.engine.http.client.heartbeat

import com.sos.scheduler.engine.common.time.ScalaTime._
import java.time.Duration

/**
  * @author Joacim Zschimmer
  */
final case class HttpHeartbeatTiming(period: Duration, timeout: Duration) {
  override def toString = s"HttpHeartbeatTiming(${period.pretty}, next request timeout ${timeout.pretty})"
}
