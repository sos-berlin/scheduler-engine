package com.sos.scheduler.engine.tests.jira.js1039

import JS1039TaskStdoutIT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.util.regex.Pattern
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.util.matching.Regex

/** JS-1039 FIXED: API functions stdout_text and stderr_text return empty strings when used in monitor of shell-job.
  * Prüft, ob Task.stdout_text und Task.stderr_text die Ausgaben vom Shell-Prozess enthalten
  * und ob die Ausgaben des Shell-Prozesses und des Monitors im Task-Log erscheinen. */
@RunWith(classOf[JUnitRunner])
final class JS1039TaskStdoutIT extends ScalaSchedulerTest {

  private lazy val tcpPort = FreeTcpPortFinder.findRandomFreePort(10000 until 20000)
  protected override lazy val testConfiguration = TestConfiguration(mainArguments = List(s"-tcp-port=$tcpPort"))

  protected override def onSchedulerActivated() {
    scheduler executeXml <process_class name="test-remote" remote_scheduler={s"127.0.0.1:$tcpPort"}/>
  }

  for (jobPath <- jobPaths)
    test(jobPath.getName) {
      runAndCheckJob(jobPath)
    }

  private def runAndCheckJob(jobPath: JobPath) {
    runJob(jobPath)
    for (what <- List("STDOUT", "STDERR")) {
      scheduler.instance[VariableSet].apply(what) should include (s"/script $what/")
      val taskLog = controller.environment.taskLogFileString(jobPath)
      shouldOccurExactlyOnce(taskLog, s"/script $what/")
      shouldOccurExactlyOnce(taskLog, s"/spooler_task_after $what/")
    }
  }

  private def runJob(jobPath: JobPath) {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      scheduler executeXml <start_job job={jobPath.string}/>
      eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == jobPath }
    }
  }

  private def shouldOccurExactlyOnce(string: String, what: String) {
    string should include (what)
    withClue(s"'$what' should occur only once: ") {
      (new Regex(Pattern.quote(what)) findAllIn string).size should equal (1)
    }
  }
}

private object JS1039TaskStdoutIT {
  private val jobPaths = List(
    JobPath.of("/test-local-shell"),
    JobPath.of("/test-remote-shell"),
    JobPath.of("/test-local-api"),
    JobPath.of("/test-remote-api"))
}
