package com.sos.scheduler.engine.tests.jira.js1513

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1513IT extends FreeSpec with ScalaSchedulerTest {

  "Order.history_id" in {
    val log = runOrder(JobChainPath("/test") orderKey "1").logString
    val id = 2
    assert(log contains s"Shell SCHEDULER_ORDER_HISTORY_ID='$id'")
    assert(log contains s"Scala history_id=$id")
    assert(log contains s"JavaScript history_id=$id")
  }
}
