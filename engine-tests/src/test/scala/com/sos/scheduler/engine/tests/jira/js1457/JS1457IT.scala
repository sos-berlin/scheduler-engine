package com.sos.scheduler.engine.tests.jira.js1457

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent}
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1457.JS1457IT._
import java.lang.System.currentTimeMillis
import java.util.concurrent.TimeoutException
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1457 High seadlock probability when starting multiple processes at one.
 * JS-1462 CreateProcess fails with MSWIN-00000020 The process cannot access the file because it is being used by another process.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1457IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", "-log-level=info"))
  private var stop = false

  "JS-1457" in {
    writeConfigurationFile(ProcessClassPath("/test-agent"), <process_class max_processes={s"$ParallelTaskCount"} remote_scheduler={s"127.0.0.1:$tcpPort"}/>)
    runJobAndWaitForEnd(JobPath("/test"))   // Smoke test
    val t = currentTimeMillis()
    var count = 0
    try
      intercept[TimeoutException] {
        eventBus.awaitingEvent2[ErrorLogEvent](TestDuration, _ ⇒ true) {
          writeConfigurationFile(ProcessClassPath("/test-agent"), <process_class max_processes={s"$ParallelTaskCount"} remote_scheduler={s"127.0.0.1:$tcpPort"}/>)
          runJobAndWaitForEnd(JobPath("/test"))   // Smoke test
          eventBus.on[TaskClosedEvent] { case _ ⇒ count += 1 }
          for (_ ← 1 to ParallelTaskCount) startJobAgainAndAgain()
        }
      }
    finally {
      stop = true
      waitForCondition(TestTimeout, 100.ms) { job(TestJobPath).state == JobState.pending }
    }
    logger.info(s"$count processes, ${count * 1000 / (currentTimeMillis() - t)} processes/s")
  }

  private def startJobAgainAndAgain(): Unit = runJobFuture(TestJobPath).result onSuccess { case _ ⇒ if (!stop) startJobAgainAndAgain() }
}

private object JS1457IT {
  private val ParallelTaskCount = 20
  private val TestDuration = 15.s
  private val TestJobPath = JobPath("/test")
  private val logger = Logger(getClass)
}
