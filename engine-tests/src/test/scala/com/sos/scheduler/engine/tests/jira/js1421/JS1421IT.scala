package com.sos.scheduler.engine.tests.jira.js1421

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1421IT extends FreeSpec with ScalaSchedulerTest {

  // PLEASE CHECK MANUALLY RUNNING CHILD PROCESSES !!!
  // Windows: PING.EXE
  // Linux: sleep

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))

  for (jobPath ← List(JobPath("/test-shell"), JobPath("/test-shell-monitor"), JobPath("/test-api"), JobPath("/test-api-monitor"))) {
    jobPath.withoutStartingSlash in {
      writeConfigurationFile(ProcessClassPath("/test-agent"), <process_class remote_scheduler={s"127.0.0.1:$tcpPort"}/>)
      val signalFile = testEnvironment.directory / "SIGNALFILE"
      val run = runJobFuture(jobPath, variables = Map("SIGNALFILE" → signalFile.toString))
      waitForCondition(TestTimeout, 100.ms) { Files.exists(signalFile) }
      sleep(1.s)  // Time to let shell process fall asleep
      controller.toleratingErrorCodes(Set(
        MessageCode("SCHEDULER-280"),
        MessageCode("SCHEDULER-202"),
        MessageCode("SCHEDULER-279"),
        MessageCode("Z-REMOTE-101"),
        MessageCode("WINSOCK-10054")))
      {
        scheduler executeXml <kill_task job={jobPath.string} id={run.taskId.string} immediately="true"/>
        awaitSuccess(run.result)(10.s)
      }
    }
  }
}
