package com.sos.scheduler.engine.tests.jira.js975

import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js975.JS975IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS975IT extends FunSuite with ScalaSchedulerTest {

  test("JS-975 Answer of <show_history job='...' what='log'/> should contain the log element") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    eventPipe.nextWithCondition[TaskEndedEvent] { _.jobPath == jobPath }
    (scheduler executeXml <show_history job={jobPath.string} what="log"/>).string should include("+++ TEXT FOR LOG +++")
  }
}

private object JS975IT {
  private val jobPath = JobPath("/test")
}
