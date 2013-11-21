package com.sos.scheduler.engine.tests.jira.js1039

import JS1039TaskStdoutTextTest._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

/** JS-1039 */
@RunWith(classOf[JUnitRunner])
final class JS1039TaskStdoutTextTest extends ScalaSchedulerTest {
  test("spooler_task_after() should have access to stdout with spooler_task.stdout_text") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == jobPath }
    val v = scheduler.instance[VariableSet]
    v("STDOUT") should include ("/STDOUT/")
  }
}

private object JS1039TaskStdoutTextTest {
  private val jobPath = JobPath.of("/test")
}
