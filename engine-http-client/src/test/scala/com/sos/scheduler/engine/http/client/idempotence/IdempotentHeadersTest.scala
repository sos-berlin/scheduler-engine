package com.sos.scheduler.engine.http.client.idempotence

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class IdempotentHeadersTest extends FreeSpec {

  "X-JobScheduler-Request-ID" in {
    val requestIdNumber = 123567890123456789L
    val requestId = RequestId(requestIdNumber)
    val name = "X-JobScheduler-Request-ID"

    val value = s"$requestIdNumber PT7S"
    val headerLine = s"$name: $value"
    val header = `X-JobScheduler-Request-ID`(requestId, 7.s)
    assert(header.toString == headerLine)

    val `X-JobScheduler-Request-ID`.Value(requestId_, duration) = value
    assert(requestId_ == requestId && duration == 7.s)
  }
}
