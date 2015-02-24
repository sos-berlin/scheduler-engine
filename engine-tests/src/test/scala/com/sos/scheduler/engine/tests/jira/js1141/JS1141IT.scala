package com.sos.scheduler.engine.tests.jira.js1141

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJobAndWaitForEnd
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1141.JS1141IT._
import org.joda.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1141 BUG: When having an include parameter file with modification date before 30 of march and creation date after 30 of march JobScheduler does not start any more.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1141IT extends FreeSpec with ScalaSchedulerTest {

  private val watchdog = new Thread {
    override def run(): Unit = {
      val timeout = 60000
      try {
        Thread.sleep(timeout)
        val msg = s"Exiting Java-VM due to non-terminating JobScheduler start after ${timeout}ms"
        logger.error(msg)
        System.err.println(msg)
        System.exit(99)
      }
      catch {
        case _: InterruptedException ⇒
      }
    }
  }

  override protected def onBeforeSchedulerActivation(): Unit = {
    // Change modification timestamps to other daylight saving time period, 4 to 8 months ago
    modifyIncludes(-1)
    watchdog.start()
  }

  onClose {
    watchdog.interrupt()
  }

  "At start, included file is from other day light saving time period" in {
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is older than include" in {
    eventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      modifyIncludes(+1)
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is newer than include" in {
    eventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      modifyIncludes(-1)
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  private def modifyIncludes(sign: Int): Unit = {
    aIncludeFile.setLastModified((now() + sign * (4*30).days).getMillis)
    bIncludeFile.setLastModified((now() + sign * (8*30).days).getMillis)
  }

  private def jobFile = testEnvironment.fileFromPath(TestJobPath)
  private def aIncludeFile = testEnvironment.configDirectory / TestAIncludeName
  private def bIncludeFile = testEnvironment.configDirectory / TestBIncludeName
}

private object JS1141IT {
  private val TestJobPath = JobPath("/test")
  private val TestAIncludeName = "test-a-include.xml"
  private val TestBIncludeName = "test-b-include.xml"
  private val logger = Logger(getClass)
}
