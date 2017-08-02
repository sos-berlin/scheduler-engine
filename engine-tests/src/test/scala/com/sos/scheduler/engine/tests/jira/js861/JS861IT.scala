package com.sos.scheduler.engine.tests.jira.js861

import com.sos.scheduler.engine.common.process.windows.{WindowsProcessCredentials, WindowsUserName}
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js861.JS861IT._
import java.util.Locale
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.util.control.NonFatal

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS861IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  if (isWindows) {
    val me = WindowsUserName(
      (sys.env collectFirst {
        case (k, v) if k.toLowerCase(Locale.ROOT) == "username" ⇒ v.toLowerCase(Locale.ROOT)
      }) getOrElse sys.error("Missing environment variable USERNAME"))

    if (sys.props contains "JS-861") {
      "Start a process with user credentials" in {
        // Windows Credential Managers must contain a generic entry named "test-target".
        val expectedUser = WindowsProcessCredentials.byKey("test-target").user
        for (result ← (for (jobChainPath ← LogonJobChainPaths) yield startOrder(jobChainPath orderKey "TEST").result) await 99.s) {
          check(result, expectedUser)
        }
      }
    }

    "Start a process without user credentials" in {
      for (result ← (for (jobChainPath ← NoLogonJobChainPaths) yield startOrder(jobChainPath orderKey "TEST").result) await 99.s) {
        check(result, me)
      }
    }

    for ((jobPath, n) ← Array(JobPath("/invalid-credentials-shell") → 50, JobPath("/invalid-credentials-api") → 3)) {
      s"Unknown credential key $jobPath" in {
        controller.toleratingErrorCodes( _ ⇒ true) {
          val closeds = for (_ ← 1 to n) yield
            startJob(jobPath).closed await TestTimeout  // No TaskEndedEvent
          // Now, logString must be fetched from database
          for (closed ← closeds) {
            //assert(taskRun.returnCode == ReturnCode(1))
            assert(taskLog(closed.taskId) contains "Windows Credential Manager does not return an entry named 'NON-EXISTING-CREDENTIALS-KEY': WINDOWS-1168 (CredRead)")
          }
        }
      }
    }

    def check(result: OrderRunResult, userName: WindowsUserName) =
      try {
        checkLog(result, userName)
        assert(result.variables("JOB-VARIABLE") == "JOB-VALUE")
      } catch { case NonFatal(t) ⇒
        throw new Exception(s"${result.orderKey}: $t", t)
      }

    def checkLog(result: OrderRunResult, userName: WindowsUserName) = {
      val log = result.logString
      assert(log.contains(s"THIS IS THE JOB ${result.orderKey.jobChainPath.string}"))  // jobPath == jobChainPath
      val userNameLines = log split "\n" filter { _ contains "TEST-USERNAME=" } map { _.trim.toLowerCase(Locale.ROOT) }
      assert(userNameLines exists { _ endsWith "self-test" })
      userName match {
        case `me` ⇒
          assert(userNameLines exists { _ endsWith ("\\" + me.string) })  // whoami outputs domain backslash username
        case _ ⇒
          assert(userNameLines forall { o ⇒ !o.endsWith("\\" + me.string) })
          // whoami exists with error ???  assert(userNameLines exists { _.endsWith("\\" + userName.string.toLowerCase(Locale.ROOT)) })  // whoami outputs domain backslash username
      }
    }
  }
}

private object JS861IT {
  private val LogonJobChainPaths = List(JobChainPath("/shell-logon")) // ??? Does not work on my computer: JobChainPath("/api-logon"), JobChainPath("/shell-with-monitor-logon"))
  private val NoLogonJobChainPaths = List(JobChainPath("/shell"), JobChainPath("/api"), JobChainPath("/shell-with-monitor"))
}
