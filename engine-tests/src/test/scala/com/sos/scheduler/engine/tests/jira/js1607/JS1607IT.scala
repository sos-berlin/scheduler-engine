package com.sos.scheduler.engine.tests.jira.js1607

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.ThreeTypesOfAgentProcessClasses
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1607IT extends FreeSpec with ScalaSchedulerTest with ThreeTypesOfAgentProcessClasses {

  "spooler_task.exit_code in monitor" - {
    addTestsForThreeTypesOfAgent(ProcessClassPath("/test")) { _ ⇒
      val result = controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
        runOrder(JobChainPath("/test") orderKey "1")
      }
      assert(result.variables("spooler_process_after_exit_code") == "0")
      assert(result.variables("spooler_task_after_exit_code") == "7")
    }
  }
}
