package com.sos.scheduler.engine.http.client.idempotence

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.common.{OwnHttpHeader, OwnHttpHeaderCompanion}
import java.time.Duration

/**
  * @author Joacim Zschimmer
  */
object IdempotentHeaders {

  final case class `X-JobScheduler-Request-ID`(id: RequestId, timeout: Duration) extends OwnHttpHeader {
    def companion = `X-JobScheduler-Request-ID`
    def value = s"${id.number} $timeout"
  }

  object `X-JobScheduler-Request-ID` extends OwnHttpHeaderCompanion {
    private val ValueRegex = s"(\\d+) ($Iso8601DurationRegex)".r

    object Value {
      def unapply(value: String): Option[(RequestId, Duration)] =
        value match {
          case ValueRegex(id, duration) ⇒ Some(RequestId.fromString(id), Duration.parse(duration))
        }
      }
  }
}
