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

  "newSecret" in {
    logger.debug(newSecret().toString)
    val result = measureTime(1000, "newSecret") { newSecret() }
    assert(result.singleDuration < 1.ms)
  }

  "Secret.toString does not show secret" in {
    assert(TunnelToken.Secret("secret").toString == "Secret(...)")
  }
}

private object TunnelTokenTest {
  private val logger = Logger(getClass)
}
