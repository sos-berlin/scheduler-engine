package com.sos.scheduler.engine.tests.jira.js1031

import JS1031IT._
import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.order.{OrderKey, OrderTouchedEvent}
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.io.File
import org.joda.time.DateTimeZone
import org.joda.time.Instant.now
import org.joda.time.format.ISODateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem

/** JS-1031 FIXED: An order with a missing schedule starts immediately after the JobScheduler starts. */
@RunWith(classOf[JUnitRunner])
final class JS1031IT extends ScalaSchedulerTest {
  override protected lazy val testConfiguration = TestConfiguration(terminateOnError = false)
  private lazy val eventPipe = controller.newEventPipe()

  override protected def checkedBeforeAll() {
    eventPipe
    super.checkedBeforeAll()
  }

  test("Missing test.schedule.xml should prevent order start") {
    intercept[EventPipe.TimeoutException] {
      eventPipe.nextKeyed[OrderTouchedEvent](testOrderKey, timeout = 5.s)
    }
    val at = now() + 5.s
    val scheduleElem =
      <schedule>
        <at at={ISODateTimeFormat.dateHourMinuteSecond withZone DateTimeZone.getDefault print at}/>
      </schedule>
    Files.write(scheduleElem.toString(), new File(controller.environment.configDirectory, "test.schedule.xml"), UTF_8)
    instance[FolderSubsystem].updateFolders()
    //controller.getEventBus.dispatchEvents()   // Nur bis v1.6 nötig
    eventPipe.nextKeyed[OrderTouchedEvent](testOrderKey)
  }
}

private object JS1031IT {
  private val testOrderKey = OrderKey.of("/test", "1")
}
