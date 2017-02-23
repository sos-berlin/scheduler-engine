package com.sos.scheduler.engine.tests.scheduler.comapi.java

import com.sos.jobscheduler.common.scalautil.AutoClosing._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogged
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderNodeTransition, OrderStepEnded}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.comapi.java.SchedulerAPIIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._
import scala.collection.immutable
import scala.concurrent.Promise

/**
 * @author Andreas Liebert
 */
@RunWith(classOf[JUnitRunner])
final class SchedulerAPIIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-include-path=$IncludePath"))

  import controller.newEventPipe

  private val finishedOrderParametersPromise = Promise[Map[String, String]]()
  private val eventsPromise = Promise[immutable.Seq[AnyKeyedEvent]]()
  private lazy val testTextFile = testEnvironment.liveDirectory / TestTextFilename
  private lazy val taskLogLines = eventsPromise.successValue collect { case KeyedEvent(_, e: InfoLogged) ⇒ e.message }

  protected override def onSchedulerActivated() = {
    scheduler executeXml VariablesJobElem
    scheduler executeXml <process_class name={TestProcessClassPath.withoutStartingSlash}
                                        remote_scheduler={agentUri}
                                        max_processes={s"$MaxProcesses"}/>
    super.onSchedulerActivated()
  }

  "sos.spooler.Log methods" - {
    for ((name, jobPath) ← List("Without Agent" → LogJobPath, "With Agent" -> LogJobPath.asAgent)) {
      name in {
        val taskResult: TaskResult = runJob(jobPath)
        for (level <- LogJob.LogMessages.keySet()) {
          taskResult.logString should include regex s"(?i)\\[$level\\]\\s+" + LogJob.LogMessages.get(level)
        }
        taskResult.logString should include(LogJob.SpoolerInitMessage)
        taskResult.logString should include(LogJob.SpoolerExitMessage)
        taskResult.logString should include(LogJob.SpoolerOpenMessage)
        taskResult.logString should include(LogJob.SpoolerCloseMessage)
        taskResult.logString should include(LogJob.StdOutMessage)
      }
    }
  }

  "sos.spooler.Job methods" in {
    val taskLog = runJob(JobObjectJobPath).logString
    val job = instance[JobSubsystemClient].job(JobObjectJobPath)

    for (mes <- JobObjectJob.UnwantedMessage.values) {
      taskLog should not include mes.toString
    }
    taskLog should include(s"include_path=$IncludePath")
    taskLog should include(s"process_class name=${TestProcessClassPath.name}")
    taskLog should include(s"process_class remote_scheduler=$agentUri")
    taskLog should include(s"process_class max_processes=$MaxProcesses")
    taskLog should include(s"title=$JobObjectJobTitle")

    job.stateText should equal(TestStateText)
  }


  "Run variables job via order" in {
    autoClosing(newEventPipe()) { eventPipe ⇒
      eventBus.onHot[OrderStepEnded] {
        case KeyedEvent(orderKey, _) ⇒
          finishedOrderParametersPromise.success(orderDetailed(orderKey).variables)
      }
      eventBus.awaiting[OrderFinished](VariablesOrderKey) {
        scheduler executeXml OrderCommand(VariablesOrderKey, parameters = Map(OrderVariable.pair, OrderParamOverridesJobParam.pair))
      }
      eventsPromise.success(eventPipe.queued[Event])
    }
  }

  "Variables job exit code" in {
    assertResult(List(OrderNodeTransition.Success)) {
      eventsPromise.successValue collect {
        case KeyedEvent(VariablesOrderKey, OrderStepEnded(stateTransition)) ⇒ stateTransition
      }
    }
  }

  "Order variable" in {
    assert(taskLogLines contains OrderVariable.expectedString)
  }

  "variables count" in {
    assert(taskLogLines contains TaskParamsCountPrefix + "2")
  }

  "variable substitution" in {
    val substitutedString = VariableSubstitutionString.replace("$" + JobParam.name, JobParam.value)
    assert(taskLogLines contains substitutedString)
  }

  "Order variable created in job" in {
    finishedOrderParametersPromise.successValue should contain(OrderVariableSetInJob.pair)
  }
}

object SchedulerAPIIT {
  private val LogJobPath = JobPath("/log")
  private val VariablesJobchainPath = JobChainPath("/variables")
  private val VariablesJobPath = JobPath("/variables")
  private val VariablesOrderKey = VariablesJobchainPath orderKey "1"
  private val MeasureTimeJobPath = JobPath("/measure_time")
  val OrderVariable = Variable("orderparam", "ORDERVALUE")
  val OrderVariableSetInJob = Variable("orderparaminjob", "qwertzui")
  val OrderParamOverridesJobParam = Variable("ORDEROVERRIDESJOBPARAM", "ORDEROVERRIDESJOBVALUE")
  val TaskParamsCountPrefix = "Taskparamscount:"
  private val JobParam = Variable("testparam", "PARAM-VALUE")
  val VariableSubstitutionString = "aaaa $"+JobParam.name+" aaaa"
  val TestTextFilename = "logText.txt"
  private val IncludePath = "fooo"
  val JobObjectJobPath = JobPath("/job_object")
  val JobObjectJobTitle = "JobObjectJobTitle"
  val RemoveMeJobPath = JobPath("/remove_me")
  val TestStateText = "This is a test state text."
  private val TestProcessClassPath = ProcessClassPath("/TEST")
  private val MaxProcesses = 23

  final case class Variable(name: String, value: String) {
    override def toString = name

    def expectedString = s"$name=$value"

    def pair = name → value
  }

  private val VariablesJobElem =
    <job name={VariablesJobPath.name} process_class={TestProcessClassPath.string} stop_on_error="false" order="yes">
      <params>
        <param name={JobParam.name} value={JobParam.value}/>
        <param name={OrderParamOverridesJobParam.name} value="OVERRIDDEN-JOB-VALUE"/>
      </params>
      <script language="java" java_class="com.sos.scheduler.engine.tests.scheduler.comapi.java.VariablesJob"/>
    </job>
}
