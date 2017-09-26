package com.sos.scheduler.engine.tests.jira.js1727

import com.sos.scheduler.engine.data.job.{JobPath, JobTaskQueueChanged}
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJob
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class EventsIT extends FreeSpec with ScalaSchedulerTest {

  "JobTaskQueueChanged" in {
    val eventPipe = controller.newEventPipe()
    runJob(JobPath("/test"))
    assert(eventPipe.queued[JobTaskQueueChanged.type].size == 2)
  }
}
