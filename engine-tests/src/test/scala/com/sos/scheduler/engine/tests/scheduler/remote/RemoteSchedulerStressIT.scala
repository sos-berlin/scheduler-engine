package com.sos.scheduler.engine.tests.scheduler.remote

import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.remote.RemoteSchedulerStressIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class RemoteSchedulerStressIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  lazy override protected val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"))

  s"Start job on remote scheduler $n times" in {
    val initialThreadCount = Thread.activeCount
    scheduler executeXml <process_class name="remote" remote_scheduler={s"127.0.0.1:$tcpPort"}/>
    for (i <- 1 to n) {
      runJobAndWaitForEnd(testJobPath)
      withClue(s"Thread count should be nearly constant after job run $i:") {
        Thread.activeCount should be (initialThreadCount +- 20)
      }
    }
  }
}

private object RemoteSchedulerStressIT {
  private val n = 30   // Kleine Zahl, damit es nicht zu  lange dauert.
  private val testJobPath = JobPath("/test-a")
}
