package com.sos.scheduler.engine.tests.scheduler.job.stdout_text

import TaskStdoutTextTest._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.scalatest.FunSuite
import org.scalatest.Matchers._

final class TaskStdoutTextTest extends FunSuite with ScalaSchedulerTest {
  test("spooler_task_after() should have access to stdout with spooler_task.stdout_text") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == jobPath }
    val v = scheduler.injector.getInstance(classOf[VariableSet])
    pendingUntilFixed { v("STDOUT") should include ("/STDOUT//") }    // Der Prozess (Windows?) puffert die Ausgabe?  TODO Fehler ist nicht behoben
  }
}

private object TaskStdoutTextTest {
  private val jobPath = JobPath.of("/test")
}