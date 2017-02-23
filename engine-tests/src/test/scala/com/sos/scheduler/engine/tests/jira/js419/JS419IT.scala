package com.sos.scheduler.engine.tests.jira.js419

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.xmlcommands.StartJobCommand
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js419.JS419IT._
import java.nio.file.Files.move
import java.nio.file.Paths
import java.time.LocalDate
import java.time.format.DateTimeFormatter.ISO_DATE
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS419IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val testIncludeFile = testEnvironment.liveDirectory / "test-include.xml"
  private lazy val jobSubsystem = instance[JobSubsystemClient]

  "Changing includes holidays file should effect job" in {
    for (_ ← 1 to 3) {
      testIncludeFile.xml =
        <holidays>
            <holiday date={LocalDate.now format ISO_DATE}/>
            <holiday date={LocalDate.now plusDays 1 format ISO_DATE}/>
        </holidays>
      runTasks()
      val jobFiles = TestJobPaths map testEnvironment.fileFromPath
      for (o ← jobFiles) move(o, Paths.get(s"$o~"))
      updateFolders()
      for (o ← TestJobPaths) assert(!(jobSubsystem contains o))
      for (o ← jobFiles) move(Paths.get(s"$o~"), o)
      updateFolders()
      startJobs() map { _.result } await 10.s
      //delete(testIncludeFile)  Job is not read when a includes file is missing.
      //runTasks()
    }
  }

  private def runTasks(): Unit = {
    updateFolders()
    val taskRuns = startJobs()
    sleep(1.s)
    assert(!Future.sequence(taskRuns map { _.started }).isCompleted)

    testIncludeFile.xml = <holidays/>
    updateFolders()
    taskRuns map { _.result } await 5.s
  }

  private def startJobs(): List[TaskRun] =
    for (jobPath ← TestJobPaths) yield startJob(StartJobCommand(jobPath, at = Some(StartJobCommand.At.Period)))

  private def updateFolders() = instance[FolderSubsystemClient].updateFolders()
}

private object JS419IT {
  private val TestJobPaths = List(JobPath("/test-unnamed-schedule"), JobPath("/test-named-schedule"))
}
