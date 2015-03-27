package com.sos.scheduler.engine.tests.jira.js1145

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1145.JS1145IT._
import java.io.File
import java.nio.file.Files
import java.util.regex.Pattern
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable
import scala.util.matching.Regex
import com.sos.scheduler.engine.common.time.ScalaJoda._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1145IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val aMonitorFile = testEnvironment.fileFromPath(MonitorPath("/test-a"))
  private lazy val sIncludeFile = testEnvironment.configDirectory / "test-s.js"
  private lazy val tIncludeFile = testEnvironment.configDirectory / "test-t.js"

  "Named monitor" in {
    runNamedMonitorJob()
  }

  "Named monitor with overridden ordering" in {
    runJob(JobPath("/test-ordering-named-monitor")) shouldEqual List(
      TestCMonitor.PreStepString, TestBMonitor.PreStepString, TestAMonitor.PreStepString,
      TestAMonitor.PostStepString, TestBMonitor.PostStepString, TestCMonitor.PostStepString)
  }

  "Anonymous monitor" in {
    runJob(JobPath("/test-anonymous-monitor")) shouldEqual List(
      TestAMonitor.PreStepString, TestBMonitor.PreStepString, TestCMonitor.PreStepString,
      TestCMonitor.PostStepString, TestBMonitor.PostStepString, TestAMonitor.PostStepString)
  }

  "Javascript monitor" in {
    runNamedJavascriptMonitorJob()
  }

  "Deleting a monitor stopps dependant job and ends its tasks" in {
    val run = runJobFuture(NamedMonitorJobPath, variables = Map(TestJob.DelayName → "30"))
    awaitSuccess(run.started)
    sleep(2.s)
    assert(run.ended.isCompleted)
    Files.move(aMonitorFile, renamed(aMonitorFile))
    updateFolders()
    assert(job(NamedMonitorJobPath).fileBasedState == FileBasedState.incomplete)
    awaitSuccess(run.ended)(5.s)
    assert(job(NamedMonitorJobPath).state == JobState.stopped)
  }

  "When all missing monitors have been appeared, the dependant job is activated again" in {
    Files.move(renamed(aMonitorFile), aMonitorFile)
    updateFolders()
    assert(job(NamedMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(job(NamedMonitorJobPath).state == JobState.pending)
    runNamedMonitorJob()
  }

  "A missing include file used by a monitor results in failed job start" in {
    Files.move(sIncludeFile, renamed(sIncludeFile))
    updateFolders()
    assert(job(JavascriptMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(job(JavascriptMonitorJobPath).state == JobState.pending)
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-399"), MessageCode("SCHEDULER-280"))) {
      runJobAndWaitForEnd(JavascriptMonitorJobPath).logString should include ("SCHEDULER-399")
    }
  }

  "When all missing include files used by monitors reappears, the dependant job runs successfully again" in {
//    Files.move(tIncludeFile, renamedTIncludeFile)
//    updateFolders()
//    assert(job(JavascriptMonitorJobPath).fileBasedState == FileBasedState.incomplete)
//    assert(job(JavascriptMonitorJobPath).state == JobState.stopped)
    Files.move(renamed(sIncludeFile), sIncludeFile)
    updateFolders()
//    assert(job(JavascriptMonitorJobPath).fileBasedState == FileBasedState.incomplete)
//    assert(job(JavascriptMonitorJobPath).state == JobState.stopped)
//    Files.move(renamedTIncludeFile, tIncludeFile)
//    updateFolders()
    assert(job(JavascriptMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(job(JavascriptMonitorJobPath).state == JobState.pending)
    runNamedJavascriptMonitorJob()
  }

  "Simple job with include starts event if included file is missing" in {
    // Just to prove that <script live_file="..."> does not inhibit a job start, if the requisite is missing.
    // So, we do not implement this for <script> in <monitor>, too.
    val jobPath = JobPath("/test-include")
    runJob(jobPath) shouldEqual List("TEST-INCLUDE.JS")
    val file = testEnvironment.configDirectory / "test-include.js"
    val renamedFile = new File(s"$file~")
    Files.move(file, renamedFile)
    updateFolders()
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-399"))) {
      runJobAndWaitForEnd(jobPath).logString should include ("SCHEDULER-399")
    }
  }

  private def updateFolders() = {
    instance[FolderSubsystem].updateFolders()
  }

  private def runNamedMonitorJob(): Unit = {
    runJob(NamedMonitorJobPath) shouldEqual List(
      TestAMonitor.PreStepString, TestCMonitor.PreStepString, TestBMonitor.PreStepString,
      TestBMonitor.PostStepString, TestCMonitor.PostStepString, TestAMonitor.PostStepString)
  }

  private def runNamedJavascriptMonitorJob(): Unit = {
    runJob(JavascriptMonitorJobPath) shouldEqual List("JAVASCRIPT PRE-STEP", "JAVASCRIPT POST-STEP")
  }

  private def runJob(jobPath: JobPath): immutable.Seq[String] =
    runJobAndWaitForEnd(jobPath).logString.split("\r?\n").toVector collect { case LogMarkerRegex(o) ⇒ o }
}

private[js1145] object JS1145IT {
  private val NamedMonitorJobPath = JobPath("/test-named-monitor")
  private val JavascriptMonitorJobPath = JobPath("/test-javascript-monitor")
  private val Start = ">>>"
  private val End = "<<<"
  private val LogMarkerRegex = new Regex(".*" + Pattern.quote(Start) + "(.*)" + Pattern.quote(End) + ".*")

  private[js1145] def mark(o: String) = Start + o + End

  private def renamed(f: File) = new File(s"$f~")
}
