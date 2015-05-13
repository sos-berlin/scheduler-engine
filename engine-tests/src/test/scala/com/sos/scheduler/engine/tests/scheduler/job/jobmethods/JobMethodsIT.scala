package com.sos.scheduler.engine.tests.scheduler.job.jobmethods

import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.JobMethodsIT._
import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.TestJob._
import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.TestMonitor._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JobMethodsIT extends FreeSpec with ScalaSchedulerTest with AgentTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))

  "Self test" in {
    val lines = """xx
      >first< CALLED
      xx
      xx >second< CALLED yy"""
    assert(calls(lines) == List("first", "second"))
  }

  "Method call sequence after a job method return false" - {
    List(
      List(SpoolerInitName) → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      List(SpoolerOpenName) → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerCloseName,
        SpoolerOnSuccessName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      List(SpoolerProcessName) → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerCloseName,
        SpoolerOnSuccessName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      Nil → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerCloseName,
        SpoolerOnSuccessName,
        SpoolerExitName,
        SpoolerTaskAfterName)
    ) foreach { case (methodsReturningFalse, expectedCalls) ⇒
      s"$methodsReturningFalse returns false" - {
        check(methodsReturningFalse map { _ → false.toString }, expectedCalls)
      }
    }
  }

  "Method call sequence after a method throws an exception" - {
    List(
      SpoolerInitName → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      SpoolerOpenName → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerCloseName,
        SpoolerOnErrorName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      SpoolerProcessName → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerCloseName,
        SpoolerOnErrorName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      SpoolerCloseName → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerCloseName,
        SpoolerOnErrorName,
        SpoolerExitName,
        SpoolerTaskAfterName),
      SpoolerExitName → List(
        SpoolerTaskBeforeName,
        SpoolerInitName,
        SpoolerOpenName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerProcessBeforeName,
        SpoolerProcessName,
        SpoolerProcessAfterName,
        SpoolerCloseName,
        SpoolerOnSuccessName,
        SpoolerExitName,
        SpoolerTaskAfterName)
    ) foreach { case (failingMethod, expectedCalls) ⇒
      s"Error in $failingMethod" - {
        check(List(failingMethod → Error), expectedCalls, toleratedErrorCodes = Set(MessageCode("COM-80020009"), MessageCode("Z-JAVA-105")))
      }
    }
  }

  private def check(variables: Iterable[(String, String)], expectedCallsIncludingMonitor: Iterable[String], toleratedErrorCodes: Set[MessageCode] = Set()): Unit =
    for ((monitorMode, monitorTestGroupName) ← List("" → "Without monitor", "monitor" → "With monitor")) {
      val expectedCalls = expectedCallsIncludingMonitor filterNot { name ⇒ monitorMode.isEmpty && TestMonitor.AllMethodNames(name) }  // Monitor-Methoden nur erwarten, wenn der Job einen Monitor hat
      s"$monitorTestGroupName => ${expectedCalls mkString ", "}" - {
        for (language ← List("java", "javascript");
             agentMode ← List("", "agent")) {
          val jobName = List(language, agentMode, monitorMode) filter { _.nonEmpty } mkString("test-", "-", "")
          jobName in {
            val taskResult = controller.toleratingErrorCodes(toleratedErrorCodes) {
              runJobAndWaitForEnd(JobPath.makeAbsolute(jobName), variables = (AllMethodsTrue ++ variables).toMap)
            }
            assert(calls(taskResult.logString) == expectedCalls)
          }
        }
      }
    }

  /** Selects all lines matching `CalledPattern` and returns a list of the names. */
  private def calls(logString: String): List[String] = {
    val r = for (line ← logString split "\n";
                 content ← (CalledPattern findFirstMatchIn line) map { _.group(1) })
            yield content
    r.toList
  }
}

object JobMethodsIT {
  private val CalledPattern = ">([a-z_]+)< CALLED".r
  private val Error = "ERROR"  // lets .toBoolean fail
  private val AllMethodsTrue = (TestJob.AllMethodNames ++ TestMonitor.AllMethodNames) map { _ → true.toString }
}
