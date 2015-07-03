package com.sos.scheduler.engine.tunnel.data

import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TunnelOverview(
  id: TunnelId,
  remoteTcpAddress: Option[String],
  statistics: TunnelStatistics)

object TunnelOverview {
  implicit val MyJsonFormat = jsonFormat3(apply)
}
