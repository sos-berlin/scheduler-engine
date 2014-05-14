package com.sos.scheduler.engine.tests.jira.js995

import JS995IT._
import com.sos.scheduler.engine.common.scalautil.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-995 */
@RunWith(classOf[JUnitRunner])
final class JS995IT extends FreeSpec with ScalaSchedulerTest {

  "Job soll erst aktiv werden, wenn benoetigter Schedule da ist" in {
    val job = instance[JobSubsystem].job(testJobPath)
    job.state shouldEqual JobState.loaded
    controller.getEventBus.awaitingKeyedEvent[FileBasedActivatedEvent](testJobPath) {
      testEnvironment.fileFromPath(testSchedulePath).xml = <schedule name={testSchedulePath.name}/>
    }
    job.state shouldEqual JobState.pending
  }
}

private object JS995IT {
  private val testSchedulePath = SchedulePath("/test-b")
  private val testJobPath = JobPath("/test-a")
}
