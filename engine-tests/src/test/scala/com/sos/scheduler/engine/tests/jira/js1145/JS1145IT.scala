package com.sos.scheduler.engine.tests.jira.js1145

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
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

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1145IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val aMonitorFile = testEnvironment.fileFromPath(MonitorPath("/subfolder/test-a"))
  private lazy val sIncludeFile = testEnvironment.liveDirectory / "subfolder" / "test-s.js"

  "Named monitor" in {
    runNamedMonitorJob()
  }

  "Named monitor with overridden ordering" in {
    runJobReturnLogLines(JobPath("/subfolder/test-ordering-named-monitor")) shouldEqual List(
      TestCMonitor.PreStepString, TestBMonitor.PreStepString, TestAMonitor.PreStepString,
      TestAMonitor.PostStepString, TestBMonitor.PostStepString, TestCMonitor.PostStepString)
  }

  "Anonymous monitor" in {
    runJobReturnLogLines(JobPath("/subfolder/test-anonymous-monitor")) shouldEqual List(
      TestAMonitor.PreStepString, TestBMonitor.PreStepString, TestCMonitor.PreStepString,
      TestCMonitor.PostStepString, TestBMonitor.PostStepString, TestAMonitor.PostStepString)
  }

  "Javascript monitor" in {
    runNamedJavascriptMonitorJob()
  }

  "Job with referencing an unknown monitor is not active" in {
    assert(jobOverview(LateMonitorJobPath).fileBasedState == FileBasedState.incomplete)
    assert(jobOverview(LateMonitorJobPath).state == JobState.loaded)
  }

  "When the monitor is defined, the job is activated" in {
    testEnvironment.fileFromPath(LateMonitorPath).xml =
      <monitor>
        <script java_class="com.sos.scheduler.engine.tests.jira.js1145.TestAMonitor"/>
      </monitor>
    updateFolders()
    assert(jobOverview(LateMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(jobOverview(LateMonitorJobPath).state == JobState.pending)
  }

  "Deleting a monitor stopps dependant job and ends its tasks" in {
    val run = startJob(NamedMonitorJobPath, variables = Map(TestJob.DelayName → "30"))
    awaitSuccess(run.started)
    sleep(2.s)
    assert(run.ended.isCompleted)
    Files.move(aMonitorFile, renamed(aMonitorFile))
    updateFolders()
    assert(jobOverview(NamedMonitorJobPath).fileBasedState == FileBasedState.incomplete)
    awaitSuccess(run.ended)(5.s)
    assert(jobOverview(NamedMonitorJobPath).state == JobState.stopped)
  }

  "When all missing monitors have been appeared, the dependant job is activated again" in {
    Files.move(renamed(aMonitorFile), aMonitorFile)
    updateFolders()
    assert(jobOverview(NamedMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(jobOverview(NamedMonitorJobPath).state == JobState.pending)
    runNamedMonitorJob()
  }

  "A missing include file used by a monitor results in failed job start" in {
    Files.move(sIncludeFile, renamed(sIncludeFile))
    updateFolders()
    assert(jobOverview(JavascriptMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(jobOverview(JavascriptMonitorJobPath).state == JobState.pending)
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-399"), MessageCode("SCHEDULER-280"))) {
      runJob(JavascriptMonitorJobPath).logString should include ("SCHEDULER-399")
    }
  }

  "When all missing include files used by monitors reappears, the dependant job runs successfully again" in {
    Files.move(renamed(sIncludeFile), sIncludeFile)
    updateFolders()
    assert(jobOverview(JavascriptMonitorJobPath).fileBasedState == FileBasedState.active)
    assert(jobOverview(JavascriptMonitorJobPath).state == JobState.pending)
    runNamedJavascriptMonitorJob()
  }

  "Simple job with include starts event if included file is missing" in {
    // Just to prove that <script live_file="..."> does not inhibit a job start, if the requisite is missing.
    // So, we do not implement this for <script> in <monitor>, too.
    val jobPath = JobPath("/subfolder/test-include")
    runJobReturnLogLines(jobPath) shouldEqual List("TEST-INCLUDE.JS")
    val file = testEnvironment.liveDirectory / "subfolder" / "test-include.js"
    val renamedFile = new File(s"$file~")
    Files.move(file, renamedFile)
    updateFolders()
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-399"))) {
      runJob(jobPath).logString should include ("SCHEDULER-399")
    }
  }

  private def updateFolders() = {
    instance[FolderSubsystemClient].updateFolders()
  }

  private def runNamedMonitorJob(): Unit = {
    runJobReturnLogLines(NamedMonitorJobPath) shouldEqual List(
      TestAMonitor.PreStepString, TestCMonitor.PreStepString, TestBMonitor.PreStepString,
      TestBMonitor.PostStepString, TestCMonitor.PostStepString, TestAMonitor.PostStepString)
  }

  private def runNamedJavascriptMonitorJob(): Unit = {
    runJobReturnLogLines(JavascriptMonitorJobPath) shouldEqual List("JAVASCRIPT PRE-STEP", "JAVASCRIPT POST-STEP")
  }

  private def runJobReturnLogLines(jobPath: JobPath): immutable.Seq[String] =
    runJob(jobPath).logString.split("\r?\n").toVector collect { case LogMarkerRegex(o) ⇒ o }
}

private[js1145] object JS1145IT {
  private val LateMonitorJobPath = JobPath("/test-late-monitor")
  private val LateMonitorPath = MonitorPath("/test-late")
  private val NamedMonitorJobPath = JobPath("/subfolder/test-named-monitor")
  private val JavascriptMonitorJobPath = JobPath("/subfolder/test-javascript-monitor")
  private val Start = ">>>"
  private val End = "<<<"
  private val LogMarkerRegex = new Regex(".*" + Pattern.quote(Start) + "(.*)" + Pattern.quote(End) + ".*")

  private[js1145] def mark(o: String) = Start + o + End

  private def renamed(f: File) = new File(s"$f~")
}
