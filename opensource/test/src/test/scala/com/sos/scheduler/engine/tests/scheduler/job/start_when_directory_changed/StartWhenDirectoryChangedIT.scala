package com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed

import StartWhenDirectoryChangedIT._
import com.google.common.io.Files.{move, touch}
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import java.lang.Thread.sleep
import org.joda.time._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.slf4j.LoggerFactory
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
class StartWhenDirectoryChangedIT extends ScalaSchedulerTest {
  private lazy val directory = new File(controller.environment().directory(), "start_when_directory_changed")
  private val startTimes = mutable.Buffer[LocalTime]()

  override def checkedBeforeAll() {
    controller.activateScheduler("-log-level=debug9")
  }

  test("start_when_directory_changed") {
    directory.mkdir()
    val file = new File(directory, "X")
    val file_ = new File(directory, "X~")
    scheduler executeXml jobElem(directory, """^.*[^~]$""")
    sleep(500)
    //startTimes should have size (1)

    touch(file_)
    logger.debug(s"$file_ touched")
    sleep(responseTime)
    startTimes should have size (0)   // X~ passt nicht zum regulären Ausdruck

    move(file_, file)
    logger.debug(s"$file moved")
    sleep(responseTime)
    startTimes should have size (1)   // X passt

    touch(new File(file+"X"))
    logger.debug(s"${file}X touched")
    sleep(responseTime)
    startTimes should have size (2)   // XX passt

    if (isWindows) {
      file.delete()
      logger.debug(s"$file deleted")
      sleep(responseTime)
      startTimes should have size (3)   // Unter Unix wird Löschen nicht berücksichtigt
    }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    if (e.jobPath == jobPath)
      startTimes += new LocalTime
  }
}

private object StartWhenDirectoryChangedIT {
  val logger = LoggerFactory.getLogger(classOf[StartWhenDirectoryChangedIT])
  val responseTime = (if (isWindows) 0 else 10000) + 2000
  val jobPath = JobPath.of("/a")

  def jobElem(directory: File, regEx: String) =
    <job name={jobPath.getName}>
      <script java_class="com.sos.scheduler.engine.test.jobs.SingleStepJob"/>
      <start_when_directory_changed directory={directory.toString} regex={regEx}/>
    </job>
}