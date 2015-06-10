package com.sos.scheduler.engine.tests.jira.js1031

import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import com.sos.scheduler.engine.common.time.JodaJavaTimeConversions.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.order.{OrderKey, OrderTouchedEvent}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1031.JS1031IT._
import java.io.File
import java.time.Instant.now
import org.joda.time.DateTimeZone
import org.joda.time.format.ISODateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

/** JS-1031 FIXED: An order with a missing schedule starts immediately after the JobScheduler starts. */
@RunWith(classOf[JUnitRunner])
final class JS1031IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)
  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation(): Unit = {
    eventPipe
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
    Files.write(scheduleElem.toString(), new File(controller.environment.liveDirectory, "test.schedule.xml"), UTF_8)
    instance[FolderSubsystem].updateFolders()
    //controller.getEventBus.dispatchEvents()   // Nur bis v1.6 nötig
    eventPipe.nextKeyed[OrderTouchedEvent](testOrderKey)
  }
}

private object JS1031IT {
  private val testOrderKey = OrderKey("/test", "1")
}
