package com.sos.scheduler.engine.tests.jira.js946.a

import com.sos.jobscheduler.common.scalautil.Futures._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js946.a.JS946IT._
import java.lang.System.currentTimeMillis
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS946IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = FreeTcpPortFinder.findRandomFreeTcpPort()

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"),
    ignoreError = Set(
      MessageCode("SCHEDULER-280"),  // "Process terminated with exit code 1"
      MessageCode("Z-REMOTE-118")))  // "Separate process pid=0: No response from new process within 60s"

  "With invalid remote_scheduler address or inaccessible remote scheduler, task does not start within a minute" in {
    scheduler executeXml <process_class name="inaccessible-remote" remote_scheduler={s"127.0.0.1:$tcpPort"}/>
    // Wir starten beide Jobs parallel, damit der Test nicht zweimal eine Minute dauert.
    val time = currentTimeMillis()
    val futures = List(InvalidRemoteJobPath, InaccessibleRemoteJobPath) map { path ⇒ startJob(path).result }
    for (f <- futures)
      awaitResult(f, 70.s)
    currentTimeMillis - time shouldBe 60000L +- 10000
  }

  "Limited job chain should continue when number of orders falls below limit" in {
    val orderKeys = 1 to jobChainOverview(TestJobChainPath).orderLimit.get + 2 map { i ⇒ TestJobChainPath orderKey i.toString }
    val futures =
      for (orderKey <- orderKeys) yield {
        val f = eventBus.eventFuture[OrderFinished](orderKey)
        scheduler executeXml OrderCommand(orderKey)
        sleep(500.ms)
        f
      }
    for (f <- futures) awaitResult(f, 15.s)
  }
}

private object JS946IT {
  private val InvalidRemoteJobPath = JobPath("/test-invalid-remote")
  private val InaccessibleRemoteJobPath = JobPath("/test-inaccessible-remote")
  private val TestJobChainPath = JobChainPath("/test-a")
}
