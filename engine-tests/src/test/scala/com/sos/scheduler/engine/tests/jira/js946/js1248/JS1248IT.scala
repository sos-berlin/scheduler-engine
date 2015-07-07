package com.sos.scheduler.engine.tests.jira.js946.js1248

import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js946.js1248.JS1248IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
 * JS-946, New microscheduling, test case of JS-1248.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1248IT extends FreeSpec with ScalaSchedulerTest {

  "15 orders and a process class with 5 processes" in {
    if (isUnix) pending
    val allFinished = Future.sequence(for (orderKey ← TestOrderKeys) yield eventBus.keyedEventFuture[OrderFinishedEvent](orderKey))
    for (orderKey ← TestOrderKeys) scheduler executeXml OrderCommand(orderKey)
    awaitSuccess(allFinished)(300.s)
  }
}

private object JS1248IT {
  private val TestJobChainPath = JobChainPath("/01_JobChainA")
  private val TestOrderKeys = for (i ← 1 to 15) yield TestJobChainPath orderKey s"$i"
}
