package com.sos.scheduler.engine.tests.jira.js1031

import com.sos.scheduler.engine.common.time.JodaJavaTimeConversions.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.order.{OrderKey, OrderTouchedEvent}
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.test.SchedulerTestUtils.writeConfigurationFile
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1031.JS1031IT._
import java.time.Instant.now
import org.joda.time.DateTimeZone
import org.joda.time.format.ISODateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** JS-1031 FIXED: An order with a missing schedule starts immediately after the JobScheduler starts. */
@RunWith(classOf[JUnitRunner])
final class JS1031IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation() = eventPipe

  "Missing test.schedule.xml should prevent order start" in {
    assert(eventPipe.queued[OrderTouchedEvent].isEmpty)
    val at = now() + 5.s
    writeConfigurationFile(TestSchedulePath,
      <schedule>
        <at at={ISODateTimeFormat.dateHourMinuteSecond withZone DateTimeZone.getDefault print at}/>
      </schedule>)
    sleep(2.s)
    assert(eventPipe.queued[OrderTouchedEvent].isEmpty)
    eventPipe.nextKeyed[OrderTouchedEvent](TestOrderKey, timeout = 5.s)
  }
}

private object JS1031IT {
  private val TestOrderKey = OrderKey("/test", "1")
  private val TestSchedulePath = SchedulePath("/test")
}
