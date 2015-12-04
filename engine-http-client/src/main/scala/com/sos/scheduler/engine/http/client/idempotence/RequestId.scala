package com.sos.scheduler.engine.http.client.idempotence

import java.util.UUID

/**
  * @author Joacim Zschimmer
  */
final case class RequestId(string: String) {
  require(!string.isEmpty && !(string contains ' '), s"Invalid RequestId: '$string'")
}

object RequestId {
  val Regex = """\p{Graph}+""".r

  def generate() = new RequestId(UUID.randomUUID().toString)
}
