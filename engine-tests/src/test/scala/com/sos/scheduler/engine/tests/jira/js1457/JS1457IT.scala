package com.sos.scheduler.engine.tests.jira.js1457

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.{JobPath, JobState, TaskClosed}
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1457.JS1457IT._
import java.lang.System.currentTimeMillis
import java.util.concurrent.TimeoutException
import java.util.concurrent.atomic.AtomicInteger
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1457 High deadlock probability when starting multiple processes at once.
 * JS-1462 CreateProcess fails with MSWIN-00000020 The process cannot access the file because it is being used by another process.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1457IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", "-log-level=info"))
  @volatile private var stop = false

  "Fixed: High deadlock probability when starting multiple processes, CreateProcess fails with MSWIN-00000020" in {
    writeConfigurationFile(ProcessClassPath("/test-agent"),
      ProcessClassConfiguration(processMaximum = Some(ParallelTaskCount), agentUris = List(AgentAddress(s"127.0.0.1:$tcpPort"))))
    runJob(JobPath("/test"))   // Smoke test
    val t = currentTimeMillis()
    val count = new AtomicInteger
    try
      intercept[TimeoutException] {
        eventBus.awaitingInTimeWhen[ErrorLogged](TestDuration, _ ⇒ true) {
          writeConfigurationFile(ProcessClassPath("/test-agent"),
            ProcessClassConfiguration(processMaximum = Some(ParallelTaskCount), agentUris = List(AgentAddress(s"127.0.0.1:$tcpPort"))))
          runJob(JobPath("/test"))   // Smoke test
          eventBus.on[TaskClosed.type] { case _ ⇒ count.incrementAndGet() }
          for (_ ← 1 to ParallelTaskCount) startJobAgainAndAgain()
        }
      }
    finally {
      stop = true
      waitForCondition(TestTimeout, 100.ms) { jobOverview(TestJobPath).state == JobState.pending }
    }
    logger.info(s"$count processes, ${count.get * 1000 / (currentTimeMillis() - t)} processes/s")
  }

  private def startJobAgainAndAgain(): Unit = for (_ ← startJob(TestJobPath).result if !stop) startJobAgainAndAgain()
}

private object JS1457IT {
  private val ParallelTaskCount = 20
  private val TestDuration = 15.s
  private val TestJobPath = JobPath("/test")
  private val logger = Logger(getClass)
}
