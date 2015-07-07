package com.sos.scheduler.engine.tests.jira.js946.js1248

import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
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

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-tcp-port=3333"),
    cppSettings = Map(CppSettingName.htmlDir → """C:\sos\all\joc\joc-html-cockpit\target\classes"""))

  "15 orders, a job with 2 tasks and a process class with 3 process" in {
    if (isUnix) pending
    runOrders(JobChainPath("/test"), 15)
  }

  "15 orders and a process class with 5 processes" in {
    if (isUnix) pending
    runOrders(JobChainPath("/01_JobChainA"), 15)
  }

  private def runOrders(jobChainPath: JobChainPath, n: Int): Unit = {
    val orderKeys = for (i ← 1 to n) yield jobChainPath orderKey s"$i"
    val allFinished = Future.sequence(for (orderKey ← orderKeys) yield eventBus.keyedEventFuture[OrderFinishedEvent](orderKey))
    for (orderKey ← orderKeys) scheduler executeXml OrderCommand(orderKey)
    awaitSuccess(allFinished)(300.s)
  }
}
