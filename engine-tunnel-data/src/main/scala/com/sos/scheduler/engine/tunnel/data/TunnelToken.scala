package com.sos.scheduler.engine.tunnel.data

import com.sos.scheduler.engine.base.generic.IsString
import com.sos.scheduler.engine.tunnel.data.TunnelToken._
import java.security.SecureRandom
import java.util.Base64
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TunnelToken(id: TunnelId, secret: Secret) {
  override def toString = id.toString
}

object TunnelToken {
  implicit val MyJsonFormat = jsonFormat2(apply)
  private val ByteCount = 18  // 144 bits (a UUID has 128 bits). For base64, a multiple of 3 bytes is good.
  private val toUrlBase64 = Base64.getUrlEncoder.encodeToString _
  private val random = new SecureRandom

  private[tunnel] def newSecret() = Secret({
    val bytes = new Array[Byte](ByteCount)
    random.nextBytes(bytes)
    toUrlBase64(bytes) stripSuffix "="
  })

  final case class Secret(string: String) extends IsString {
    override def toString = "Secret(...)"
  }

  object Secret extends IsString.HasJsonFormat[Secret]
}
