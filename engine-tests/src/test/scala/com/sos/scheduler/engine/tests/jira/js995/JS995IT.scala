package com.sos.scheduler.engine.tests.jira.js995

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js995.JS995IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-995 */
@RunWith(classOf[JUnitRunner])
final class JS995IT extends FreeSpec with ScalaSchedulerTest {

  "Job soll erst aktiv werden, wenn benoetigter Schedule da ist" in {
    def job = instance[JobSubsystemClient].jobOverview(testJobPath)
    job.state shouldEqual JobState.loaded
    eventBus.awaitingKeyedEvent[FileBasedActivated.type](testJobPath) {
      testEnvironment.fileFromPath(testSchedulePath).xml = <schedule name={testSchedulePath.name}/>
    }
    job.state shouldEqual JobState.pending
  }
}

private object JS995IT {
  private val testSchedulePath = SchedulePath("/test-b")
  private val testJobPath = JobPath("/test-a")
}
