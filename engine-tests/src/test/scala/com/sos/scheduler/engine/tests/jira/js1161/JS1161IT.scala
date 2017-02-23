package com.sos.scheduler.engine.tests.jira.js1161

import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1161.JS1161IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1161IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val port = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(testClass, mainArguments = List(s"-tcp-port=$port"))

  override protected def onSchedulerActivated(): Unit = {
    scheduler executeXml <process_class name="remote" remote_scheduler={s"127.0.0.1:$port"}/>
  }

  "Job with three live_include in own scheduler" in {
    runJob(TestJobPath)
    testEnvironment.taskLogFileString(TestJobPath).replaceAll("[\r\n]", " ") should
      fullyMatch regex ".*LOCAL-START.*ONE.*MIDDLE.*TWO.*THREE.*LOCAL-END.*".r
  }

  "Job with three live_include in remote scheduler" in {
    runJob(RemoteJobPath)
    testEnvironment.taskLogFileString(RemoteJobPath).replaceAll("[\r\n]", " ") should
      fullyMatch regex ".*REMOTE-START.*ONE.*MIDDLE.*TWO.*THREE.*REMOTE-END.*".r
  }
}

private object JS1161IT {
  private val TestJobPath = JobPath("/test-local")
  private val RemoteJobPath = JobPath("/test-remote")
}
