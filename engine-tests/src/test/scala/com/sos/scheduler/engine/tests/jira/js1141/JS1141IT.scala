package com.sos.scheduler.engine.tests.jira.js1141

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJob
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1141.JS1141IT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1141 BUG: When having an include parameter file with modification date before 30 of march and creation date after 30 of march JobScheduler does not start any more.
 *
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
    runJob(TestJobPath)
  }

  "Job is older than include" in {
    eventBus.awaiting[FileBasedActivated.type](TestJobPath) {
      modifyIncludes(+1)
      Thread.sleep(2500)
      instance[FolderSubsystemClient].updateFolders()
    }
    runJob(TestJobPath)
  }

  "Job is newer than include" in {
    eventBus.awaiting[FileBasedActivated.type](TestJobPath) {
      modifyIncludes(-1)
      Thread.sleep(2500)
      instance[FolderSubsystemClient].updateFolders()
    }
    runJob(TestJobPath)
  }

  private def modifyIncludes(sign: Int): Unit = {
    aIncludeFile.setLastModified((now() + sign * (4*30) * 24.h).toEpochMilli)
    bIncludeFile.setLastModified((now() + sign * (8*30) * 24.h).toEpochMilli)
  }

  private def aIncludeFile = testEnvironment.liveDirectory / TestAIncludeName
  private def bIncludeFile = testEnvironment.liveDirectory / TestBIncludeName
}

private object JS1141IT {
  private val TestJobPath = JobPath("/test")
  private val TestAIncludeName = "test-a-include.xml"
  private val TestBIncludeName = "test-b-include.xml"
  private val logger = Logger(getClass)
}
