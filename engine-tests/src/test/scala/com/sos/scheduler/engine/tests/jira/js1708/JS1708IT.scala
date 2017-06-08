package com.sos.scheduler.engine.tests.jira.js1708

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.soslicense.LicenseKeyParameterIsMissingException
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
  * JS-1708 Agent without LicenseKey limits parallel started tasks too.
  * <p>
  * JS-1683 Agent without LicenseKey correctly counts task release.
  * <p>
  * JS-1417 JobScheduler Universal Agent checks license from all master keys.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1708IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(getClass, terminateOnError = false)

  "Agent accepts only one running task" in {
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    var taskResults = List[Future[TaskResult]]()
    eventBus.awaitingWhen[ErrorLogged](_.event.message contains classOf[LicenseKeyParameterIsMissingException].getSimpleName) {
      for (_ ← 1 to 2) taskResults ::= startJob(JobPath("/test")).result
    }
    // Scheduler delays by 30s: SCHEDULER-489  No remote JobScheduler is accessible. Waiting before trying again
    Future.firstCompletedOf(taskResults) await TestTimeout
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
    runJob(JobPath("/test"))
  }
}
