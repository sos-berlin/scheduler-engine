package com.sos.scheduler.engine.tests.jira.js1463

import com.sos.jobscheduler.common.system.OperatingSystem.isUnix
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.job.ReturnCode
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1463.JS1463IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1463 Job timeout kills a shell task after the specified time
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1463IT extends FreeSpec with ScalaSchedulerTest {

  "job timeout" in {
    val result = controller.toleratingErrorCodes(Set(
      MessageCode("SCHEDULER-272"),   // Terminating task after reaching deadline <job timeout="2">
      MessageCode("SCHEDULER-280"),   // Process terminated with exit code 99 (0x63)
      MessageCode("SCHEDULER-279")))  // Unix: Process terminated with signal 9 (SIGKILL Kill, unblockable)
    {
      runJob(TestJobPath)
    }
    assert(result.duration >= 2.s)  // One second shorter than the timeout, for some test machines (namely gimli)
    assert(result.returnCode == ReturnCode(if (isUnix) -9 else 99))  // -9: SIGKILL
  }
}

private object JS1463IT {
  private val TestJobPath = JobPath("/test")
}
