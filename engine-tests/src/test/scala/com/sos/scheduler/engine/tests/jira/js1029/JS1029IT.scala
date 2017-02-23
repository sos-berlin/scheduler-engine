package com.sos.scheduler.engine.tests.jira.js1029

import com.sos.jobscheduler.common.system.OperatingSystem.isUnix
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskKey, TaskStarted}
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1029.JS1029IT._
import java.nio.file.Files.delete
import java.nio.file.Path
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

/**
 * JS-1029 Log is filled with SCHEDULER-478 when temporary file for a jobs stdout is removed.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1029IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)

  if (isUnix) {  // Nur unter Unix kann eine offene Datei gelöscht werden
    test("JobScheduler should handle removed stdout file") {
      val eventPipe = controller.newEventPipe()
      scheduler executeXml <start_job job={testJobPath.string}/>
      val KeyedEvent(taskId, MyTaskStartedEvent(stdoutFile)) = eventPipe.nextAny[MyTaskStartedEvent]
      try {
        delete(stdoutFile)
        killTask(taskId)
        eventPipe.next[TaskEnded](TaskKey(testJobPath, taskId))
      }
      finally
        killTask(taskId)
    }
  }

  private def killTask(taskId: TaskId): Unit = {
    scheduler executeXml <kill_task job={testJobPath.string} id={taskId.string} immediately="yes"/>
  }

  eventBus.onHot[TaskStarted.type] {
    case KeyedEvent(TaskKey(_, taskId), _) ⇒
    eventBus publishCold KeyedEvent(MyTaskStartedEvent(taskDetailed(taskId).stdoutFile))(taskId)
  }
}

private object JS1029IT {
  private val testJobPath = JobPath("/test")

  private case class MyTaskStartedEvent(stdoutFile: Path) extends Event {
    type Key = TaskId
  }
}
