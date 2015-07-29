package com.sos.scheduler.engine.tests.jira.js1329

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, InfoLogEvent}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1329IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    errorLogEventIsTolerated = _.message contains "TEST-STDERR")

  "stderr may be logged with log level error" in {
    eventBus.awaitingEvent[InfoLogEvent](_.message contains "TEST-STDOUT") {
      eventBus.awaitingEvent[ErrorLogEvent](_.message contains "TEST-STDERR") {
        runJobAndWaitForEnd(JobPath("/test"))
      }
    }
  }
}
