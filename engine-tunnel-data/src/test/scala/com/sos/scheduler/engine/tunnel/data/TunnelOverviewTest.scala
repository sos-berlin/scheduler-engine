package com.sos.scheduler.engine.tunnel.data

import java.time.Instant
import org.scalatest.FreeSpec
import spray.json.{pimpAny, pimpString}

/**
 * @author Joacim Zschimmer
 */
final class TunnelOverviewTest extends FreeSpec {

  "JSON" in {
    val obj = TunnelOverview(
      TunnelId("TUNNEL-1"),
      remoteTcpAddress = Some("REMOTE-ADDRESS"),
      TunnelStatistics(
        requestCount = 10,
        messageByteCount = 1000,
        currentRequestIssuedAt = Some(Instant.parse("2015-07-03T12:00:00Z")),
        failure = Some("FAILURE")))
    val json = """{
        "id": "TUNNEL-1",
        "remoteTcpAddress": "REMOTE-ADDRESS",
        "statistics": {
          "requestCount": 10,
          "messageByteCount": 1000,
          "currentRequestIssuedAt": "2015-07-03T12:00:00Z",
          "failure": "FAILURE"
        }
      }""".parseJson
    assert(obj.toJson == json)
    assert(obj == json.convertTo[TunnelOverview])
  }
}
