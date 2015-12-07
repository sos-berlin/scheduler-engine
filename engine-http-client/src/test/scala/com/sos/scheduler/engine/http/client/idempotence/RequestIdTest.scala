package com.sos.scheduler.engine.http.client.idempotence

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class RequestIdTest extends FreeSpec {

  "Generator" in {
    val newRequestId = new RequestId.Generator
    assert(newRequestId() == RequestId(1))
    assert(newRequestId() == RequestId(2))
  }

  "Eater" in {
    val eat = new RequestId.Eater
    assert(eat(RequestId(11)))
    assert(eat(RequestId(12)))
    assert(!eat(RequestId(12)))
    assert(eat(RequestId(13)))
    assert(!eat(RequestId(15)))
    assert(eat.expectedId == RequestId(14))
  }
}
