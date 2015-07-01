package com.sos.scheduler.engine.tunnel.data

import com.sos.scheduler.engine.data.base.IsString
import com.sos.scheduler.engine.tunnel.data.TunnelToken._
import java.util.Base64
import scala.util.Random
import spray.json.DefaultJsonProtocol._

/**
 * @author Joacim Zschimmer
 */
final case class TunnelToken(id: TunnelId, password: Password)

object TunnelToken {
  implicit val MyJsonFormat = jsonFormat2(apply)

  private val PasswordLength = 20

  private[tunnel] def newPassword() = Password({
    val bytes = new Array[Byte](PasswordLength)
    Random.nextBytes(bytes)
    Base64.getUrlEncoder.encodeToString(bytes)
  })

  final case class Password(string: String) extends IsString {
    override def toString = "Password(...)"
  }

  object Password extends IsString.HasJsonFormat[Password]
}

