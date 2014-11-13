package com.sos.scheduler.engine.tests.jira.js1141

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJobAndWaitForEnd
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1141.JS1141IT._
import java.lang.System.currentTimeMillis
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1141 BUG: When having an include parameter file with modification date before 30 of march and creation date after 30 of march JobScheduler does not start any more.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1141IT extends FreeSpec with ScalaSchedulerTest {

  "(normal)" in {
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is older than include" in {
    controller.getEventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      jobFile.setLastModified(currentTimeMillis - 10000)
      includeFile.setLastModified(currentTimeMillis)
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  "Job is newer than include" in {
    controller.getEventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestJobPath) {
      jobFile.setLastModified(currentTimeMillis)
      includeFile.setLastModified(currentTimeMillis - 10000)
      Thread.sleep(2500)
      instance[FolderSubsystem].updateFolders()
    }
    runJobAndWaitForEnd(TestJobPath)
  }

  private def jobFile = testEnvironment.fileFromPath(TestJobPath)
  private def includeFile = testEnvironment.configDirectory / TestIncludeName
}

private object JS1141IT {
  private val TestJobPath = JobPath("/test")
  private val TestIncludeName = "test-include.xml"
}
