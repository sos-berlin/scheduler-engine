package com.sos.scheduler.engine.agenttest

import com.sos.scheduler.engine.agent.Main
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agenttest.AgentIT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{JobPath, ResultCode, TaskEndedEvent}
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable
import scala.concurrent.Await
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val agentTcpPort = findRandomFreeTcpPort()
  private lazy val agentApp = new Main(AgentConfiguration(httpPort = agentTcpPort, httpInterfaceRestriction = Some("127.0.0.1"))).closeWithCloser
  private var events: immutable.Seq[Event] = null
  private lazy val shellOutput = taskLogLines collect { case ScriptOutputRegex(o) ⇒ o.trim }
  private lazy val taskLogLines = events collect { case e: InfoLogEvent ⇒ e.message }

  protected override def onSchedulerActivated(): Unit = {
    val started = agentApp.start()
    scheduler executeXml TestJobElem
    scheduler executeXml <process_class name="test-agent" remote_scheduler={s"http://127.0.0.1:$agentTcpPort"}/>
    Await.result(started, 5.seconds)
  }

  // TODO Umgebungsvariablen, Parameter übergeben, Auftragsvariablen ändern, Stdout und Stderr periodisch ins Task-Protokoll übernehmen, result code, viele Tasks parallel und nacheinander

  "Run shell job" in {
    AutoClosing.autoClosing(controller.newEventPipe()) { eventPipe ⇒
      controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
        // "Process terminated with exit code 42"
        runJobAndWaitForEnd(TestJobPath)
      }
      events = eventPipe.queued[Event]
    }
  }

  "Shell script exit code" in {
    assertResult(List(TestResultCode)) {
      events collect { case TaskEndedEvent(_, TestJobPath, resultCode) ⇒ resultCode }
    }
    controller.eventBus.dispatchEvents()
  }

  "Job parameter" in {
    shellOutput
    assert(shellOutput contains s"$ParamName=$ParamValue")
  }

  "Job environment variable" in {
    assert(shellOutput contains s"$EnvName=$EnvValue")
  }

  "Other environment variables are unchanged" in {
    val path = (sys.env collectFirst { case ("PATH" | "Path", v) ⇒ v }).head
    assert(shellOutput contains s"PATH=$path")
  }

  "stdout in task log" in {
    assert(shellOutput contains "STDOUT AGENT ECHO")
  }

  "stderr in task log" in {
    assert(shellOutput contains "STDERR AGENT ECHO")
  }

  "Job.stateText" in {
    pending
    //job(TestJobPath).stateText shouldEqual s"!$FirstStdoutLine"
  }
}

private object AgentIT {
  private val TestJobPath = JobPath("/test")
  private val TestResultCode = ResultCode(42)
  private val FirstStdoutLine = "FIRST STDOUT LINE"
  private val EnvName = "TESTENV"
  private val EnvValue = "ENV-VALUE"
  private val ParamName = "TESTPARAM"
  private val ParamValue = "PARAM-VALUE"
  private val ScriptOutputRegex = "[^!]*!(.*)".r  // Our test script output start with '!'

  private val TestJobElem =
    <job name={TestJobPath.name} process_class="test-agent" stop_on_error="false">
      <params>
        <param name={ParamName} value={ParamValue}/>
      </params>
      <environment>
        <variable name={EnvName} value={EnvValue}/>
      </environment>
      <script language="shell">{
        if (isWindows) s"""
          |@echo off
          |echo !$FirstStdoutLine
          |echo !$ParamName=%SCHEDULER_PARAM_$ParamName%
          |echo !$EnvName=%$EnvName%
          |echo !PATH=%PATH%
          |echo !STDOUT AGENT ECHO
          |echo !STDERR AGENT ECHO 1>&2
          |exit ${TestResultCode.value}""".stripMargin
        else s"""
          |echo !$FirstStdoutLine
          |echo !$ParamName=$$SCHEDULER_PARAM_$ParamName
          |echo !$EnvName=$$$EnvName
          |echo !PATH=$$PATH
          |echo !STDOUT AGENT ECHO
          |echo !STDERR AGENT ECHO 1>&2
          |exit ${TestResultCode.value}""".stripMargin
      }</script>
    </job>
}
