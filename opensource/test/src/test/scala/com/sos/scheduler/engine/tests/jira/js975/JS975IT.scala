package com.sos.scheduler.engine.tests.jira.js975

import JS975IT._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.scalatest.matchers.ShouldMatchers._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS975IT extends ScalaSchedulerTest {
  override lazy val testConfiguration = TestConfiguration(database = Some(DefaultDatabaseConfiguration()))

  test("JS-975 Answer of <show_history job='...' what='log'/> should contain the log element") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    eventPipe.nextWithCondition[TaskEndedEvent] { _.jobPath == jobPath }
    (scheduler executeXml <show_history job={jobPath.string} what="log"/>).string should include("+++ TEXT FOR LOG +++")
  }
}

private object JS975IT {
  private val jobPath = JobPath.of("/test")
}
