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

  "newPassword" in {
    logger.debug(newPassword().toString)
    val result = measureTime(1000, "newPassword") { newPassword() }
    assert(result.singleDuration < 1.ms)
  }

  "Password.toString does not show password" in {
    assert(TunnelToken.Password("secret").toString == "Password(...)")
  }
}

private object TunnelTokenTest {
  private val logger = Logger(getClass)
}
