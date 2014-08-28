package com.sos.scheduler.engine.tests.jira.js994.two

import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS994twoIT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)
  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation(): Unit = {
    eventPipe
  }

  test("Sich einander referenzierende schedule sollen abgewiesen werden") {
    eventPipe.nextAny[ErrorLogEvent].message should startWith("SCHEDULER-463")
  }
}
