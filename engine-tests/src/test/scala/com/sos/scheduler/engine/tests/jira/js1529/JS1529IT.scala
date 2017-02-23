package com.sos.scheduler.engine.tests.jira.js1529

import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.ThreeTypesOfAgentProcessClasses
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1529.JS1529IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1529IT extends FreeSpec with ScalaSchedulerTest with ThreeTypesOfAgentProcessClasses {

  addTestsForThreeTypesOfAgent(ProcessClassPath("/test")) { title ⇒
    runJob(TestJobPath, Map("A" → title))
    val expected = s"JOB STATE TEXT $title"
    assert(job(TestJobPath).stateText == expected)
    val answer = (scheduler executeXml <show_job job={TestJobPath.string}/>).answer
    assert((answer \ "job" \ "@state_text").toString == expected)
  }
}

private object JS1529IT {
  private val TestJobPath = if (isWindows) JobPath("/test-windows") else JobPath("/test-unix")
}
