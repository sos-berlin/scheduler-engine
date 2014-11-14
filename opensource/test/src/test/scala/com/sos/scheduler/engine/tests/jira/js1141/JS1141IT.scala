package com.sos.scheduler.engine.tests.jira.js1141

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJobAndWaitForEnd
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1141.JS1141IT._
import java.lang.System.currentTimeMillis
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

  override protected def onBeforeSchedulerActivation(): Unit = {
    // Change modification timestamps to other daylight saving time period, 4 to 8 months ago
    aIncludeFile.setLastModified((now() - (4*30).days).getMillis)
    bIncludeFile.setLastModified((now() - (8*30).days).getMillis)
  }

  "At start, included file is from other day light saving time period" in {
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is older than include" in {
    controller.getEventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      jobFile.setLastModified(currentTimeMillis() - 10000)
      aIncludeFile.setLastModified(currentTimeMillis())
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is newer than include" in {
    controller.getEventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      jobFile.setLastModified(currentTimeMillis())
      aIncludeFile.setLastModified(currentTimeMillis() - 10000)
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  private def jobFile = testEnvironment.fileFromPath(TestJobPath)
  private def aIncludeFile = testEnvironment.configDirectory / TestAIncludeName
  private def bIncludeFile = testEnvironment.configDirectory / TestBIncludeName
}

private object JS1141IT {
  private val TestJobPath = JobPath("/test")
  private val TestAIncludeName = "test-a-include.xml"
  private val TestBIncludeName = "test-b-include.xml"
}
