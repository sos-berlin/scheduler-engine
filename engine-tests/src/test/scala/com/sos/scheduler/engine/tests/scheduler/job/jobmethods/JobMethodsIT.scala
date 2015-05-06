package com.sos.scheduler.engine.tests.scheduler.job.jobmethods

import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.JobMethodsIT._
import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.TestJob._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JobMethodsIT extends FreeSpec with ScalaSchedulerTest with AgentTest {

  "Self test" in {
    val lines = """xx
      >first< CALLED
      xx
      xx >second< CALLED yy"""
    assert(calls(lines) == List("first", "second"))
  }

  "Method call sequence after a job method return false" - {
    List(
      Some(SpoolerInitName) → List(SpoolerInitName, SpoolerExitName),
      Some(SpoolerOpenName) → List(SpoolerInitName, SpoolerOpenName, SpoolerCloseName, SpoolerOnSuccessName, SpoolerExitName),
      Some(SpoolerProcessName) → List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerCloseName, SpoolerOnSuccessName, SpoolerExitName),
      None → List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerProcessName, SpoolerCloseName, SpoolerOnSuccessName, SpoolerExitName)
    ) foreach { case (methodsReturningFalse, expectedCalls) ⇒
      s"$methodsReturningFalse returns false => ${expectedCalls mkString ", "}" - {
        check(methodsReturningFalse map { _ → false.toString }, expectedCalls)
      }
    }
  }

  "Method call sequence after a method throws an exception" - {
    List(
      SpoolerInitName → List(SpoolerInitName, SpoolerExitName),
      SpoolerOpenName → List(SpoolerInitName, SpoolerOpenName, SpoolerCloseName, SpoolerOnErrorName, SpoolerExitName),
      SpoolerProcessName → List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerCloseName, SpoolerOnErrorName, SpoolerExitName),
      SpoolerCloseName → List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerProcessName, SpoolerCloseName, SpoolerOnErrorName, SpoolerExitName),
      SpoolerExitName → List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerProcessName, SpoolerCloseName, SpoolerOnSuccessName, SpoolerExitName)
    ) foreach { case (failingMethod, expectedCalls) ⇒
      s"Error in $failingMethod" - {
        check(Map(failingMethod → Error), expectedCalls, toleratedErrorCodes = Set(MessageCode("COM-80020009")))
      }
    }
  }

  private def check(variables: Iterable[(String, String)], expectedCalls: Iterable[String], toleratedErrorCodes: Set[MessageCode] = Set()): Unit = {
    for ((name, jobPath) ← Settings) {
      name in {
        val taskResult = controller.toleratingErrorCodes(toleratedErrorCodes) {
          runJobAndWaitForEnd(jobPath, variables = (AllNamesTrue ++ variables).toMap)
        }
        assert(calls(taskResult.logString) == expectedCalls)
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
  private val Settings = List(
    "Java without Agent" → JobPath("/test-java-non-agent"),
    "Java with Agent" -> JobPath("/test-java-agent"),
    "JavaScript without Agent" → JobPath("/test-javascript-non-agent"),
    "JavaScript with Agent" -> JobPath("/test-javascript-agent"))
  private val CalledPattern = ">([a-z_]+)< CALLED".r
  private val Error = "ERROR"  // lets .toBoolean fail
  private val AllNames = List(SpoolerInitName, SpoolerOpenName, SpoolerProcessName, SpoolerProcessName, SpoolerCloseName, SpoolerOnSuccessName, SpoolerExitName)
  private val AllNamesTrue = AllNames map { _ → true.toString }
}
