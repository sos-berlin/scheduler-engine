package com.sos.scheduler.engine.tests.scheduler.comapi.job.start_task

import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class StartTaskIT extends FunSuite with ScalaSchedulerTest {

  test("job.start_task") {
    controller.waitForTermination()
  }

  @EventHandler def handle(e: TaskEndedEvent): Unit = {
    if (e.jobPath == JobPath("/test-b")) {
      instance[SchedulerVariableSet].apply("test-b") should equal ("TEST-TEST")
      controller.terminateScheduler()
    }
  }
}
