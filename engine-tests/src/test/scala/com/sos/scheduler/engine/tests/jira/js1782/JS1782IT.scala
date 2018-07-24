package com.sos.scheduler.engine.tests.jira.js1782

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1191 Order.last_error
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1782IT extends FreeSpec with ScalaSchedulerTest
{
  "test" in {
    val orderKeys = for (i ← 1 to 3) yield JobChainPath("/test") orderKey s"TEST-$i"
    val finished = for (o ← orderKeys) yield startOrder(o).finished
    finished await 99.s  // idle_timeout="60" should not delay order execution
  }
}
