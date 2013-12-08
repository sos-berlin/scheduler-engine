package com.sos.scheduler.engine.tests.jira.js994.one

import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class JS994oneIT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(terminateOnError = false)
  private lazy val eventPipe = controller.newEventPipe()

  protected override def checkedBeforeAll() {
    eventPipe
  }

  test("Ein sich selbst referenzierendes schedule soll abgewiesen werden") {
    eventPipe.nextAny[ErrorLogEvent].getLine should (startWith ("SCHEDULER-428") and include ("SCHEDULER-486"))
  }
}
