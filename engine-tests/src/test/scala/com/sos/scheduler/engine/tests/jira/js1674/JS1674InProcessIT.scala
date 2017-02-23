package com.sos.scheduler.engine.tests.jira.js1674

import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.Stopwatch
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.{InMemoryDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1674.JS1674InProcessIT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1674InProcessIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(tcpPort, httpPort) = findRandomFreeTcpPorts(2)  // tcpPort is for scheduler.xml supervisor_client
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", s"-http-port=$httpPort", "-log-level=debug9"),
    database = Some(InMemoryDatabaseConfiguration))

  "job language=in_process" in {
    val jobRun = startJob(InternalJobPath)
    val startedAt = now
    while (!jobRun.result.isCompleted) {
      val t = now
      scheduler executeXml <show_state/>
      withClue("Engine has not responded in time. ") {
        assert(now - t < DelayMaximum)
      }
      assert(now < startedAt + 30.s)
      sleepUntil(t + DelayMaximum)
    }
    assert(jobRun.result.successValue.returnCode.isSuccess)
  }

  "Multiple job starts" in {
    Stopwatch.measureTime(50, "InProcessJob") {
      val result = runJob(QuickJobPath)
      assert(result.returnCode.isSuccess)
    }
  }
}

private object JS1674InProcessIT {
  private val DelayMaximum = 500.ms
  private val InternalJobPath = JobPath("/InProcess")
  private val QuickJobPath = JobPath("/Quick")
}
