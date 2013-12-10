package com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed

import StartWhenDirectoryChangedIT._
import com.google.common.base.Splitter
import com.google.common.io.Files.{move, touch}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.eventbus.{SchedulerEventBus, HotEventHandler}
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.test.EventPipe.TimeoutException
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class StartWhenDirectoryChangedIT extends ScalaSchedulerTest {

  private lazy val directory = new File(controller.environment.directory, "start_when_directory_changed")
  private lazy val file1 = new File(directory, "X")
  private lazy val file2 = new File(directory, "XX")
  private lazy val file_ = new File(directory, "X~")
  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation() {
    directory.mkdir()
    eventPipe
  }

  override def onSchedulerActivated() {
    scheduler executeXml jobElem(directory, """^.*[^~]$""")
  }

  test("Filename not matching the pattern") {
    touch(file_)
    logger debug s"$file_ touched"
    intercept[TimeoutException] { eventPipe.nextWithTimeoutAndCondition[TriggerEvent](responseTime) { _ => true } }
  }

  test("Matching filename (1)") {
    move(file_, file1)
    logger debug s"$file1 moved"
    eventPipe.nextAny[TriggerEvent] should equal (TriggerEvent(Set(file1)))
  }

  test("Matching filename (2)") {
    touch(file2)
    logger debug s"$file2 touched"
    eventPipe.nextAny[TriggerEvent] should equal (TriggerEvent(Set(file1, file2)))
  }

  if (isWindows) {   // Unter Unix wird Löschen nicht berücksichtigt
    test("Matching filename deleted") {
      file1.delete()
      logger debug s"$file1 deleted"
      eventPipe.nextAny[TriggerEvent] should equal (TriggerEvent(Set(file2)))
    }
  }

  @HotEventHandler def handleEvent(e: TaskEndedEvent, task: Task) {
    e.jobPath should equal (aJobPath)
    val files = (Splitter on ";" split task.parameterValue(triggeredFilesName) map { o => new File(o) }).toSet
    instance[SchedulerEventBus] publishCold TriggerEvent(files)
  }
}


object StartWhenDirectoryChangedIT {
  private val logger = Logger(getClass)
  private val responseTime = (if (isWindows) 0.s else 10.s) + 4.s
  private val aJobPath = JobPath.of("/a")
  val triggeredFilesName = "triggeredFiles"

  private def jobElem(directory: File, regex: String) =
    <job name={aJobPath.getName}>
      <script java_class="com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed.AJob"/>
      <start_when_directory_changed directory={directory.toString} regex={regex}/>
    </job>

  private case class TriggerEvent(files: Set[File]) extends Event
}
