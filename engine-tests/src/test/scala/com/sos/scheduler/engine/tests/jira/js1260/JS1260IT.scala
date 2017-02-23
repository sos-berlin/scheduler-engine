package com.sos.scheduler.engine.tests.jira.js1260

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1260.JS1260IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1260IT extends FreeSpec with ScalaSchedulerTest {

  import testEnvironment.schedulerLog

  "JS1260IT" in {
    controller.toleratingErrorCodes(Set(ErrorCode)) {
      runJob(TestJobPath)
    }
    val firstPart = schedulerLog.contentString
    firstPart should include (UnwantedString)  // Here it's okay

    scheduler executeXml <show_job job={TestJobPath.string} what="run_time job_params task_history"/>
    val secondPart = schedulerLog.contentString stripPrefix firstPart
    secondPart should not include UnwantedString
  }
}

private object JS1260IT {
  private val TestJobPath = JobPath("/test")
  private val ErrorCode = MessageCode("SCHEDULER-280")
  private val UnwantedString = s"[ERROR $ErrorCode"
}
