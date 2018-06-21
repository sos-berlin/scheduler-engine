package com.sos.scheduler.engine.tests.jira.js1776

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.io.File.pathSeparator
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1191 Order.last_error
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1776IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "job attribute java_class_path= is ignored" - {
    "Without Agent" in {
      check(JobPath("/test"))
    }

    "With Agent" in {
      check(JobPath("/test-agent"))
    }
  }

  private def check(jobPath: JobPath): Unit = {
    val log = runJob(jobPath).logString
    assert(log contains "java.class.path=TEST-JOB-CLASS-PATH" + pathSeparator +
      sys.props("java.class.path").stripSuffix(pathSeparator) +
      "<--")
  }
}
