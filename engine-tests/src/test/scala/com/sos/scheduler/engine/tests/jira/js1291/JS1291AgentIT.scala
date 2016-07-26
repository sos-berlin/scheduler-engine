package com.sos.scheduler.engine.tests.jira.js1291

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.{RichPath, _}
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode, TaskEndedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderStepEndedEvent}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, ProcessClassConfiguration}
import com.sos.scheduler.engine.eventbus.EventSourceEvent
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1291.JS1291AgentIT._
import java.nio.file.Files
import java.nio.file.Files.{createTempFile, deleteIfExists}
import java.time.Duration
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable
import scala.concurrent.Promise

/**
 * JS-1291 First Agent tests.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1291AgentIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  import controller.{newEventPipe, toleratingErrorCodes, toleratingErrorLogEvent}
  private lazy val List(tcpPort, httpPort) = findRandomFreeTcpPorts(2)
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", s"-http-port=$httpPort"))

  List(
    //"With TCP C++ Agent" → { () ⇒ ProcessClassConfiguration(agentUris = List(s"127.0.0.1:$tcpPort"), processMaximum = Some(1000)) },
    "With Universal Agent" → { () ⇒ ProcessClassConfiguration(agentUris = List(agentUri), processMaximum = Some(1000)) })
  .foreach { case (testGroupName, lazyProcessClassConfig) ⇒
    testGroupName - {
      val eventsPromise = Promise[immutable.Seq[Event]]()
      lazy val taskLogLines = (eventsPromise.successValue collect { case e: InfoLogEvent ⇒ e.message split "\r?\n" }).flatten
      lazy val shellOutput: immutable.Seq[String] = taskLogLines collect { case ScriptOutputRegex(o) ⇒ o.trim }
      val finishedOrderParametersPromise = Promise[Map[String, String]]()

      "(prepare process class)" in {
        deleteAndWriteConfigurationFile(TestProcessClassPath, lazyProcessClassConfig())
      }

      "Run shell job via order" in {
        scheduler executeXml newJobElem()
        autoClosing(newEventPipe()) { eventPipe ⇒
          toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) { // "Process terminated with exit code ..."
            val orderKey = TestJobchainPath orderKey testGroupName
            eventBus.onHotEventSourceEvent[OrderStepEndedEvent] {
              case EventSourceEvent(event, order: Order) ⇒ finishedOrderParametersPromise.trySuccess(order.variables)
            }
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
              scheduler executeXml OrderCommand(orderKey, parameters = Map(OrderVariable.pair, OrderParamOverridesJobParam.pair))
            }
          }
          eventsPromise.success(eventPipe.queued[Event])
        }
      }

      "Shell script exit code" in {
        assertResult(List(TestReturnCode)) {
          eventsPromise.successValue collect { case TaskEndedEvent(_, TestJobPath, returnCode) ⇒ returnCode }
        }
        eventBus.dispatchEvents()
      }

      "Order variable" in {
        assert(shellOutput contains OrderVariable.expectedString)
      }

      "Order variable overrides job variable with same name" in {
        assert(shellOutput contains OrderParamOverridesJobParam.expectedString)
      }

      "Job parameter" in {
        assert(shellOutput contains JobParam.expectedString)
      }

      "Job environment variable" in {
        assert(shellOutput contains EnvironmentVariable.expectedString)
      }

      "Other environment variables are unchanged" in {
        val path = (sys.env collectFirst { case ("PATH" | "Path", v) ⇒ v }).head
        assert(shellOutput contains s"""PATH="$path"""")
      }

      "stdout in task log" in {
        assert(shellOutput contains "STDOUT AGENT ECHO")
      }

      "stderr in task log" in {
        assert(shellOutput contains "STDERR AGENT ECHO")
      }

      "Job.stateText" in {
        assert(job(TestJobPath).stateText == s"!$FirstStdoutLine")
      }

      "SCHEDULER_RETURN_VALUES" in {
        finishedOrderParametersPromise.successValue should contain(ChangedVariable.pair)
      }

      "SCHEDULER_ID" in {
        val id = instance[SchedulerId]
        assert(id.nonEmpty)
        assert(shellOutput contains s"""SCHEDULER_ID="$id"""")
      }

      "Shell with monitor has access to stdout_text" in {
        awaitSuccess(startJob(JobPath("/no-crash")).result).logString should include ("SPOOLER_PROCESS_AFTER")
      }

      "Shell with monitor - handling unexpected process termination of monitor" in {
        val file = createTempFile("sos", ".tmp") withCloser Files.delete
        toleratingErrorCodes(UnexpectedProcessTerminationErrorCodes) {
          val run = startJob(JobPath("/crash"), variables = Map(SignalName → file.toString))
          file.append("x")
          awaitSuccess(run.result).logString should include ("SCHEDULER-202")
        }
      }

      "Shell with monitor - unexpected process termination of one monitor does not disturb the other task" in {
        val file = createTempFile("sos", ".tmp") withCloser Files.delete
        toleratingErrorCodes(UnexpectedProcessTerminationErrorCodes) {
          val noCrash = startJob(JobPath("/no-crash"), variables = Map(SignalName → file.toString))
          awaitSuccess(startJob(JobPath("/crash"), variables = Map(SignalName → file.toString)).result).logString should include ("SCHEDULER-202")
          awaitSuccess(noCrash.result).logString should include ("SPOOLER_PROCESS_AFTER")
        }
      }

      "Task log contains stdout and stderr of a shell script with monitor" in {
        toleratingErrorCodes(Set(MessageCode("SCHEDULER-202"), MessageCode("SCHEDULER-280"), MessageCode("WINSOCK-10054"), MessageCode("ERRNO-32"), MessageCode("Z-REMOTE-101"))) {
          val logString = awaitSuccess(startJob(JobPath("/no-crash")).result).logString
          logString should include ("SPOOLER_PROCESS_AFTER")
          logString should include ("TEXT FOR STDOUT")
          logString should include ("TEXT FOR STDERR")
        }
      }

      "Exception in Monitor" in {
        toleratingErrorLogEvent({ e ⇒ (e.codeOption contains MessageCode("SCHEDULER-280")) || (e.message startsWith "COM-80020009") && (e.message contains "MONITOR EXCEPTION") }) {
        //toleratingErrorLogEvent({ e ⇒ (e.codeOption contains MessageCode("SCHEDULER-280")) || (e.message startsWith "COM-80020009 java.lang.RuntimeException: MONITOR EXCEPTION") }) {
          runJob(JobPath("/throwing-monitor"))
        }
      }

      "Run multiple tasks simultaneously" - {
        lazy val terminateFile = testEnvironment.directory.toPath / "TERMINATE"

        def runTasks(jobPath: JobPath, taskCount: Int, taskMaximumDuration: Duration): Unit = {
          deleteIfExists(terminateFile)
          val runs = for (jobPath ← List.fill(taskCount) { jobPath }) yield
            startJob(jobPath, variables = Map("terminate" → s"$terminateFile"))
          runs map { _.started } await taskCount * 20.s
          logger.info("All tasks are running now")
          touch(terminateFile)
          val results = runs map { _.result } await TestTimeout
          assert((results filterNot { _.returnCode.isSuccess }) == Nil)
        }

        val shellTaskCount = 100
        s"Start $shellTaskCount simultaneously running shell tasks" in {
          runTasks(JobPath("/test-sleep"), shellTaskCount, taskMaximumDuration = 1.s)
        }

        val javaTaskCount = 10
        s"Start $javaTaskCount simultaneously running API tasks" in {
          runTasks(JobPath("/test-sleep-api"), javaTaskCount, taskMaximumDuration = 20.s)  // 1 (out of 8) processor thread Intel 3770K (2012): 6s per task
        }
      }
    }
  }

  "Universal Agent sos.spooler API characteristics" in {
    runJob(JobPath("/test-api"))
  }
}

object JS1291AgentIT {
  private val TestProcessClassPath = ProcessClassPath("/test")
  private val TestJobchainPath = JobChainPath("/test")
  private val TestJobPath = JobPath("/test")
  private val TestReturnCode = ReturnCode(42)
  private val FirstStdoutLine = "FIRST STDOUT LINE"
  private val OrderVariable = Variable("orderparam", "ORDERVALUE")
  private val OrderParamOverridesJobParam = Variable("ORDEROVERRIDESJOBPARAM", "ORDEROVERRIDESJOBVALUE")
  private val JobParam = Variable("testparam", "PARAM-VALUE")
  private val EnvironmentVariable = Variable("TESTENV", "ENV-VALUE")
  private val SchedulerVariables = List(OrderVariable, OrderParamOverridesJobParam, JobParam)
  private val ScriptOutputRegex = "[^!]*!(.*)".r  // Our test script output starts with '!'
  private val ChangedVariable = Variable("CHANGED", "CHANGED-VALUE")
  val SignalName = "signalFile"
  private val UnexpectedProcessTerminationErrorCodes = Set(
    MessageCode("SCHEDULER-202"),
    MessageCode("SCHEDULER-280"),
    MessageCode("Z-REMOTE-101"),
    MessageCode("ERRNO-32"),
    MessageCode("ERRNO-131"),  // Solaris
    MessageCode("WINSOCK-10053"),
    MessageCode("WINSOCK-10054"))
  private val logger = Logger(getClass)

  private case class Variable(name: String, value: String) {
    override def toString = name
    def expectedString = s"$name=$value"
    def pair = name → value
  }

  private def newJobElem() =
    <job name={TestJobPath.name} process_class={TestProcessClassPath.withoutStartingSlash} stop_on_error="false">
      <params>
        <param name={JobParam.name} value={JobParam.value}/>
        <param name={OrderParamOverridesJobParam.name} value="OVERRIDDEN-JOB-VALUE"/>
      </params>
      <environment>
        <variable name={EnvironmentVariable.name} value={EnvironmentVariable.value}/>
      </environment>
      <script language="shell">{
        def variableToEcho(v: Variable) = {
          import v._
          val envName = paramToEnvName(name)
          "echo !" + (if (isWindows) s"$name=%$envName%" else s"$name=$$$envName") + "\n"
        }
        (
          if (isWindows) s"""
            |@echo off
            |echo !$FirstStdoutLine
            |echo !$EnvironmentVariable=%$EnvironmentVariable%
            |echo !PATH="%Path%"
            |echo !SCHEDULER_ID="%SCHEDULER_ID%"
            |if "%SCHEDULER_RETURN_VALUES%" == "" goto :noReturnValues
            |    echo ${ChangedVariable.name}=${ChangedVariable.value} >> %SCHEDULER_RETURN_VALUES%
            |:noReturnValues
            |""".stripMargin
          else s"""
            |echo !$FirstStdoutLine
            |echo !$EnvironmentVariable=$$$EnvironmentVariable
            |echo !PATH=\\""$$PATH"\\"
            |echo !SCHEDULER_ID=\\""$$SCHEDULER_ID"\\"
            |[ -n "$$SCHEDULER_RETURN_VALUES" ] && echo ${ChangedVariable.name}=${ChangedVariable.value} >> $$SCHEDULER_RETURN_VALUES
            |""".stripMargin
        ) +
        (SchedulerVariables map variableToEcho).mkString + s"""
          |echo !STDOUT AGENT ECHO
          |echo !STDERR AGENT ECHO >&2
          |exit ${TestReturnCode.toInt}
          |""".stripMargin
      }</script>
    </job>

  private def paramToEnvName(name: String) = s"SCHEDULER_PARAM_${name.toUpperCase}"
}
