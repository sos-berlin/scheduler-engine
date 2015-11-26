package com.sos.scheduler.engine.http.client.heartbeat

import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.http.client.common.{OwnHttpHeader, OwnHttpHeaderCompanion}
import java.time.Duration
import spray.http.HttpHeader

/**
  * @author Joacim Zschimmer
  */
object HeartbeatHeaders {

  private val DurationRegex = "[0-9.A-Za-z]+".r
  private val TimesRegex = s"($DurationRegex) +($DurationRegex)".r

  private def parseTimes(string: String) = string match {
    case TimesRegex(duration, timeout) ⇒ HttpHeartbeatTiming(Duration parse duration, Duration parse timeout)
  }

  final case class `X-JobScheduler-Heartbeat-Start`(timing: HttpHeartbeatTiming) extends OwnHttpHeader {
    def companion = `X-JobScheduler-Heartbeat-Start`
    def value = s"${timing.period} ${timing.timeout}"
  }
  object `X-JobScheduler-Heartbeat-Start` extends OwnHttpHeaderCompanion {
    object Value {
      def unapply(value: String): Some[HttpHeartbeatTiming] = Some(parseTimes(value))
    }
  }

  final case class `X-JobScheduler-Heartbeat-Continue`(heartbeatId: HeartbeatId, timing: HttpHeartbeatTiming) extends OwnHttpHeader {
    def companion = `X-JobScheduler-Heartbeat-Continue`
    def value = s"${heartbeatId.string} ${timing.period} ${timing.timeout}"
  }
  object `X-JobScheduler-Heartbeat-Continue` extends OwnHttpHeaderCompanion {
    private val ValueRegex = s"(${HeartbeatId.Regex}) +($DurationRegex +$DurationRegex)".r

    object Value {
      def unapply(value: String): Option[(HeartbeatId, HttpHeartbeatTiming)] =
        value match {
          case ValueRegex(heartbeatId, timing) ⇒ Some(HeartbeatId(heartbeatId), parseTimes(timing))
        }
      }
  }

  final case class `X-JobScheduler-Heartbeat`(heartbeatId: HeartbeatId) extends OwnHttpHeader {
    def companion = `X-JobScheduler-Heartbeat`
    def value = s"${heartbeatId.string}"
  }
  object `X-JobScheduler-Heartbeat` extends OwnHttpHeaderCompanion {
    def unapply(h: HttpHeader): Option[HeartbeatId] =
      h.name.toLowerCase == lowercaseName option {
        val Value(id) = h.value
        id
      }

    object Value {
      def unapply(value: String): Some[HeartbeatId] = Some(HeartbeatId(value))
    }
  }
}
