package com.sos.scheduler.engine.tests.scheduler.comapi.job.start_task

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskKey}
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

  eventBus.on[TaskEnded] {
    case KeyedEvent(TaskKey(JobPath("/test-b"), _), _) ⇒
      instance[SchedulerVariableSet].apply("test-b") should equal ("TEST-TEST")
      controller.terminateScheduler()
  }
}
