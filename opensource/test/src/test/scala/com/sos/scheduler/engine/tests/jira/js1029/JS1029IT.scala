package com.sos.scheduler.engine.tests.jira.js1029

import JS1029IT._
import com.sos.scheduler.engine.common.system.Files.removeFile
import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskId, TaskEndedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1029IT extends ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(terminateOnError = false)

  if (isUnix) {  // Nur unter Unix kann eine offene Datei gelöscht werden
    test("JobScheduler should handle removed stdout file") {
      val eventPipe = controller.newEventPipe()
      scheduler executeXml <start_job job={testJobPath.string}/>
      val MyTaskStartedEvent(taskId, stdoutFile) = eventPipe.nextAny[MyTaskStartedEvent]
      try {
        removeFile(stdoutFile)
        killTask(taskId)
        eventPipe.nextKeyed[TaskEndedEvent](taskId)
      }
      finally
        killTask(taskId)
    }
  }

  private def killTask(taskId: TaskId) {
    scheduler executeXml <kill_task job={testJobPath.string} id={taskId.string} immediately="yes"/>
  }

  @HotEventHandler def handleEvent(e: TaskStartedEvent, task: Task) {
    controller.getEventBus publishCold MyTaskStartedEvent(e.taskId, task.stdoutFile)
  }
}

private object JS1029IT {
  private val testJobPath = JobPath.of("/test")

  private case class MyTaskStartedEvent(taskId: TaskId, stdoutFile: File) extends Event
}
