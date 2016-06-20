package com.sos.scheduler.engine.tunnel.data

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch.measureTime
import com.sos.scheduler.engine.tunnel.data.TunnelToken._
import com.sos.scheduler.engine.tunnel.data.TunnelTokenTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class TunnelTokenTest extends FreeSpec {

  "newSecret returns different secrets" in {
    logger.debug(newSecret().string)
    val n = 100000
    val secrets = for (_ ← 1 to n) yield newSecret()
    assert(secrets.distinct.size == n)
    assert(secrets.map(_.string).distinct.size == n)
    assert(secrets.distinct == secrets)
  }

  "newSecret returns restricted character set" in {
    for (_ ← 1 to 1000) {
      val secretString = newSecret().string
      assert(secretString forall ExpectedCharacters)
      assert(secretString.size == SecretSize)
    }
  }

  "newSecret is fast" in {
    val result = measureTime(10000, "newSecret") { newSecret() }
    assert(result.singleDuration < 1.ms)
  }

  "Secret.toString does not show secret" in {
    assert(TunnelToken.Secret("secret").toString == "Secret(...)")
  }
}

private object TunnelTokenTest {
  private val logger = Logger(getClass)
  private val ExpectedCharacters = Set('-', '_') ++ ('A' to 'Z') ++ ('a' to 'z') ++ ('0' to '9')
  private val SecretSize = 24   // = 28/3*4
}
