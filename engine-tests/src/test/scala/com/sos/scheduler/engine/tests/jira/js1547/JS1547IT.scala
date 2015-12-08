package com.sos.scheduler.engine.tests.jira.js1547

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1547IT extends FreeSpec with ScalaSchedulerTest {

  "show_state shows process in process class" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val runs = for (j ← List(JobPath("/test-shell"), JobPath("/test-api"))) yield runJobFuture(j)
      eventPipe.nextWithCondition[InfoLogEvent](_.message contains "SCHEDULER-918  state=starting")
      eventPipe.nextWithCondition[InfoLogEvent](_.message contains "SCHEDULER-918  state=starting")
      val answer = (scheduler executeXml <show_state subsystems='process_class' what='folders'/>).answer
      val processes = answer \ "state" \ "folder" \ "process_classes" \ "process_class" \ "processes" \ "process"
      assert((processes flatMap { _.attribute("job") map { _.toString }}).toSet == Set("test-shell", "test-api"))
    }
  }
}
