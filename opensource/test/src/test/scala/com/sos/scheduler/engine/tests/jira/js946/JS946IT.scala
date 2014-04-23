package com.sos.scheduler.engine.tests.jira.js946

import JS946IT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.lang.System.currentTimeMillis
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await

@RunWith(classOf[JUnitRunner])
final class JS946IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = FreeTcpPortFinder.findRandomFreeTcpPort()

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"),
    ignoreError = Set(
      "SCHEDULER-280",  // "Process terminated with exit code 1"
      "Z-REMOTE-118"))  // "Separate process pid=0: No response from new process within 60s"

  "With invalid remote_scheduler address or inaccessible remote scheduler, task does not start within a minute" in {
    scheduler executeXml <process_class name="inaccessible-remote" remote_scheduler={s"127.0.0.1:$tcpPort"}/>
    // Wir starten beide Jobs parallel, damit der Test nicht zweimal eine Minute dauert.
    val time = currentTimeMillis()
    val futures = List(invalidRemoteJobPath, inaccessibleRemoteJobPath) map { path ⇒ runJobFuture(path)._2 }
    for (f <- futures)
      Await.result(f, 70.s)
    currentTimeMillis - time shouldBe 60000L +- 10000
  }
}

private object JS946IT {
  private val invalidRemoteJobPath = JobPath("/test-invalid-remote")
  private val inaccessibleRemoteJobPath = JobPath("/test-inaccessible-remote")
}
