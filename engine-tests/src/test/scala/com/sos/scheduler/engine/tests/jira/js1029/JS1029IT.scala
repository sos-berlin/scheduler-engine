package com.sos.scheduler.engine.tests.jira.js1029

import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskId, TaskKey, TaskStarted}
import com.sos.scheduler.engine.eventbus.EventSourceEvent
import com.sos.scheduler.engine.kernel.job.Task
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
        eventPipe.nextKeyed[TaskEnded](TaskKey(testJobPath, taskId))
      }
      finally
        killTask(taskId)
    }
  }

  private def killTask(taskId: TaskId): Unit = {
    scheduler executeXml <kill_task job={testJobPath.string} id={taskId.string} immediately="yes"/>
  }

  eventBus.onHotEventSourceEvent[TaskStarted.type] {
    case KeyedEvent(TaskKey(_, taskId), EventSourceEvent(_, task: Task)) ⇒
    eventBus publishCold KeyedEvent(MyTaskStartedEvent(task.stdoutFile))(taskId)
  }
}

private object JS1029IT {
  private val testJobPath = JobPath("/test")

  private case class MyTaskStartedEvent(stdoutFile: Path) extends Event {
    type Key = TaskId
  }
}
