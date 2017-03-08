package com.sos.scheduler.engine.tests.jira.js1031

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.{AutoClosing, Logger}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStarted}
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.test.{EventPipe, SchedulerTestUtils}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{deleteConfigurationFile, writeConfigurationFile}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1031.JS1031IT._
import java.time.Instant.now
import java.time.{Duration, Instant, ZoneId}
import java.time.format.DateTimeFormatter.ISO_LOCAL_DATE_TIME
import java.time.temporal.ChronoField.MILLI_OF_SECOND
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** JS-1031 FIXED: An order with a missing schedule starts immediately after the JobScheduler starts. */
@RunWith(classOf[JUnitRunner])
final class JS1031IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val eventPipe = controller.newEventPipe()

  override def onBeforeSchedulerActivation() = eventPipe

  "JS-1031 Missing test.schedule.xml prevents order start" in {
    logger.info(s"Time zone: ${instance[ZoneId]}")
    changeScheduleXml(3.s, eventPipe)
  }

  "JS-1694 Change of test.scheduler.xml is respected" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      changeScheduleXml(2.s, eventPipe)
    }
  }

  "JS-1694 Deletion and re-insertion of test.scheduler.xml is respected" in {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      deleteConfigurationFile(TestSchedulePath)
      sleep(1.s)
      changeScheduleXml(2.s, eventPipe)
    }
  }

  private def changeScheduleXml(delayStart: Duration, eventPipe: EventPipe): Unit = {
    assert(eventPipe.queued[OrderStarted.type].isEmpty)
    val at = now() + delayStart `with` (MILLI_OF_SECOND, 0)
    val scheduleElem = <schedule><at at={ISO_LOCAL_DATE_TIME withZone instance[ZoneId] format at}/></schedule>
    logger.info(s"$scheduleElem")
    writeConfigurationFile(TestSchedulePath, scheduleElem)
    sleepUntil(at - 1.s)
    logger.info("Order should not yet be touched")
    assert(eventPipe.queued[OrderStarted.type].isEmpty)
    eventPipe.next[OrderStarted.type](TestOrderKey, timeout = at + 1.s - now)
  }
}

private object JS1031IT {
  private val logger = Logger(getClass)
  private val TestOrderKey = OrderKey("/test", "1")
  private val TestSchedulePath = SchedulePath("/test")
}
