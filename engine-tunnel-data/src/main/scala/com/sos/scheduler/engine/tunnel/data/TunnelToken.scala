package com.sos.scheduler.engine.tunnel.data

import com.sos.scheduler.engine.data.base.IsString
import com.sos.scheduler.engine.tunnel.data.TunnelToken._
import java.util.Base64
import scala.util.Random
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TunnelToken(id: TunnelId, secret: Secret) {
  override def toString = id.toString
}

object TunnelToken {
  implicit val MyJsonFormat = jsonFormat2(apply)

  private val SecretLength = 20

  private[tunnel] def newSecret() = Secret({
    val bytes = new Array[Byte](SecretLength)
    Random.nextBytes(bytes)
    Base64.getUrlEncoder.encodeToString(bytes)
  })

  final case class Secret(string: String) extends IsString {
    override def toString = "Secret(...)"
  }

  object Secret extends IsString.HasJsonFormat[Secret]
}

