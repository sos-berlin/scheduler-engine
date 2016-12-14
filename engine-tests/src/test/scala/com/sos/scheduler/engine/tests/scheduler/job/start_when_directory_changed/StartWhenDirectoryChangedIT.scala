package com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed

import com.google.common.base.Splitter
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{KeyedEvent, NoKeyEvent}
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskKey}
import com.sos.scheduler.engine.data.xmlcommands.ModifyJobCommand
import com.sos.scheduler.engine.data.xmlcommands.ModifyJobCommand.Cmd.{Stop, Unstop}
import com.sos.scheduler.engine.test.EventPipe.TimeoutException
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed.StartWhenDirectoryChangedIT._
import java.nio.file.Files.{delete, exists, move}
import java.nio.file.{Path, Paths}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class StartWhenDirectoryChangedIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val directory = controller.environment.subdirectory("start_when_directory_changed")
  private lazy val file1 = directory / "X"
  private lazy val file2 = directory / "XX"
  private lazy val file_ = directory / "X~"
  private lazy val eventPipe = controller.newEventPipe()

  override protected def onBeforeSchedulerActivation() = {
    eventPipe
  }

  override protected def onSchedulerActivated(): Unit = {
    scheduler executeXml jobElem(directory, """^.*[^~]$""")
  }

  eventBus.onHot[TaskEnded] {
    case KeyedEvent(TaskKey(AJobPath, taskId), _) ⇒
      val files = (Splitter on ";" split taskDetailed(taskId).variables(TriggeredFilesName) map { o ⇒ Paths.get(o) }).toSet
      eventBus publishCold TriggerEvent(files)
  }

  "Filename not matching the pattern" in {
    touch(file_)
    logger debug s"$file_ touched"
    intercept[TimeoutException] { eventPipe.nextWhen[TriggerEvent](_ ⇒ true, ResponseTime) }
  }

  "Matching filename (1)" in {
    move(file_, file1)
    logger debug s"$file1 moved"
    eventPipe.nextAny[TriggerEvent].event shouldEqual TriggerEvent(Set(file1))
  }

  "Matching filename (2)" in {
    touch(file2)
    logger debug s"$file2 touched"
    eventPipe.nextAny[TriggerEvent].event shouldEqual TriggerEvent(Set(file1, file2))
  }

  if (isWindows) {   // Unter Unix wird Löschen nicht berücksichtigt
    "Matching filename deleted" in {
      delete(file1)
      logger debug s"$file1 deleted"
      eventPipe.nextAny[TriggerEvent].event shouldEqual TriggerEvent(Set(file2))
    }
  }

  "File watching when job is unstopped" in {
    scheduler executeXml ModifyJobCommand(AJobPath, cmd=Some(Stop))
    if (exists(file1)) delete(file1)
    scheduler executeXml ModifyJobCommand(AJobPath, cmd=Some(Unstop))
    touch(file1)
    logger debug s"$file1 touched"
    eventPipe.nextAny[TriggerEvent].event shouldEqual TriggerEvent(Set(file1, file2))
  }
}


object StartWhenDirectoryChangedIT {
  private val logger = Logger(getClass)
  private val ResponseTime = (if (isWindows) 0.s else 10.s) + 4.s
  private val AJobPath = JobPath("/a")
  private[start_when_directory_changed] val TriggeredFilesName = "triggeredFiles"

  private def jobElem(directory: Path, regex: String) =
    <job name={AJobPath.name}>
      <script java_class="com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed.AJob"/>
      <start_when_directory_changed directory={directory.toString} regex={regex}/>
    </job>

  private case class TriggerEvent(files: Set[Path]) extends NoKeyEvent
}
