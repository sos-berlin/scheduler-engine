package com.sos.scheduler.engine.tests.scheduler.comapi.job.start_task

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class StartTaskTest extends ScalaSchedulerTest {
  test("job.start_task") {
    controller.waitForTermination(shortTimeout)
  }

  @EventHandler def handle(e: TaskEndedEvent) {
    if (e.jobPath == JobPath.of("/test-b")) {
      scheduler.instance[VariableSet].apply("test-b") should equal ("TEST-TEST")
      controller.terminateScheduler()
    }
  }
}
