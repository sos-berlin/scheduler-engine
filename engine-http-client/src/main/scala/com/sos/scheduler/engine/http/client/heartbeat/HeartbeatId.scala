package com.sos.scheduler.engine.http.client.heartbeat

import java.util.UUID

/**
  * @author Joacim Zschimmer
  */
final case class HeartbeatId(string: String) {
  require(!(string contains ' '), s"Invalid HeartbeatId: $string")
}

object HeartbeatId {
  val Regex = """\p{Graph}+""".r

  def generate() = new HeartbeatId(UUID.randomUUID().toString)
}
