package com.sos.scheduler.engine.http.client.heartbeat

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatHeaders._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import spray.http.HttpHeader
import spray.http.HttpHeaders.`Cache-Control`

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class HeartbeatHeadersTest extends FreeSpec {

  "X-JobScheduler-Heartbeat-Start" in {
    val name = "X-JobScheduler-Heartbeat-Start"
    val value = "PT12.3S PT30S"
    val headerLine = s"$name: $value"
    val header = `X-JobScheduler-Heartbeat-Start`(HttpHeartbeatTiming(period = 12300.ms, timeout = 30.s))
    assert(header.toString == headerLine)

    val `X-JobScheduler-Heartbeat-Start`.Value(times) = value
    assert(times == HttpHeartbeatTiming(period = 12300.ms, timeout = 30.s))
  }

  "X-JobScheduler-Heartbeat-Continue" in {
    val name = "X-JobScheduler-Heartbeat-Continue"
    val heartbeatId = HeartbeatId.generate()
    val value = s"${heartbeatId.string} PT10S PT30S"
    val headerLine = s"$name: $value"
    val header = `X-JobScheduler-Heartbeat-Continue`(heartbeatId, HttpHeartbeatTiming(period = 10.s, timeout = 30.s))
    assert(header.toString == headerLine)

    val `X-JobScheduler-Heartbeat-Continue`.Value(heartbeatId_, times) = value
    assert(heartbeatId_ == heartbeatId)
    assert(times == HttpHeartbeatTiming(period = 10.s, timeout = 30.s))
  }

  "X-JobScheduler-Heartbeat" in {
    val name = "X-JobScheduler-Heartbeat"
    val heartbeatId = HeartbeatId.generate()
    val value = heartbeatId.string
    val headerLine = s"$name: $value"
    val header = `X-JobScheduler-Heartbeat`(heartbeatId)
    assert(header.toString == headerLine)

    val `X-JobScheduler-Heartbeat`.Value(heartbeatId_) = value
    assert(heartbeatId_ == heartbeatId)
    List[HttpHeader](`Cache-Control`(Nil), header) collect { case `X-JobScheduler-Heartbeat`(id) ⇒ id } shouldEqual List(heartbeatId)
  }
}
