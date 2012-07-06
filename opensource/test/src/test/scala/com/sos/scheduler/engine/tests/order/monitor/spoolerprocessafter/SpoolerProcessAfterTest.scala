package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskId, TaskClosedEvent}
import com.sos.scheduler.engine.data.log.{LogEvent, LogLevel}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{EventHandler, HotEventHandler}
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.slf4j.LoggerFactory
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class SpoolerProcessAfterTest extends ScalaSchedulerTest {
  import SpoolerProcessAfterTest._

  private val messageCodes = new mutable.HashMap[LogLevel, mutable.Set[String]]() with mutable.MultiMap[LogLevel, String]

  controller.setTerminateOnError(false)

  Settings.list.zipWithIndex foreach { case ((setting, expected), i) =>
    val index = i + 1
    test(index +". "+ setting +" should result in " +expected) {
      new MyTest(index, setting, expected)
    }
  }

  private class MyTest(index: Int, setting: Setting, expected: Expected) {
    val eventPipe = controller.newEventPipe()

    try {
      val myFinishedEvent = executeOrder()
      removeOrderAndEndTask()
      check(setting, expected, myFinishedEvent)
    }
    finally cleanUp(setting)

    def executeOrder() = {
      scheduler executeXml setting.orderElem
      eventPipe.next[MyFinishedEvent]
    }

    def removeOrderAndEndTask() {
      tryRemoveOrder(setting.orderKey)  // Damit Job nicht gleich nochmal mit demselben Auftrag startet
      scheduler executeXml <modify_job job={setting.jobPath.asString} cmd="end"/>
      eventPipe.next[TaskClosedEvent].getId should equal (new TaskId(index))
    }

    def check(setting: Setting, expected: Expected, event: MyFinishedEvent) {
      def checkOrderState() {
        if (!expected.orderStateExpectation.isValid(event.state))
          fail("Expected OrderState="+expected.orderStateExpectation+", but was "+event.state)
      }

      def checkSpoolerProcessParameter() {
        val spoolerProcessAfterParameterOption = expected.details collectFirst { case SpoolerProcessAfterParameter(o) => o }
        if (spoolerProcessAfterParameterOption != event.spoolerProcessAfterParameterOption)
          fail("Expected parameter for spooler_process_after is "+spoolerProcessAfterParameterOption +" but was "+event.spoolerProcessAfterParameterOption)
      }

      def checkJobIsStopped() {
        val isStopped = scheduler.instance[JobSubsystem].job(setting.jobPath).state == JobState.stopped
        if (isStopped != (expected.details contains JobIsStopped))
          fail("Job has"+(if (isStopped) "" else " not") +" been stopped")
      }

      def checkMessageCodes() {
        messageCodes.toMap foreach { case (level, codes) =>
          codes.toSet should equal ((expected.details collect { case MessageCode(`level`, code) => code }).toSet)
        }
      }

      event.orderKey should equal (setting.orderKey)
      checkOrderState()
      checkSpoolerProcessParameter()
      checkJobIsStopped()
      checkMessageCodes()
    }

    private def cleanUp(setting: Setting) {
      scheduler executeXml <modify_job job={setting.jobPath.asString} cmd="unstop"/>
      messageCodes.clear()
      //logger.info((scheduler executeXml <show_state/>).toString)
    }
  }

  private def tryRemoveOrder(o: OrderKey) {
    try scheduler executeXml <remove_order job_chain={o.jobChainPathString} order={o.idString}/>
    catch { case _: SchedulerException => }
  }

  @HotEventHandler def handleEvent(e: OrderStepEndedEvent, order: UnmodifiableOrder) {
    if (e.stateTransition == OrderStateTransition.keepState) {
      // Es wird kein OrderFinishedEvent geben.
      controller.getEventBus.publishCold(MyFinishedEvent(
        e.getKey, order.getState,
        Option(emptyToNull(order.getParameters.get(SpoolerProcessAfterNames.parameter))) map { _.toBoolean }))
    }
  }

  @HotEventHandler def handleEvent(e: OrderFinishedEvent, order: UnmodifiableOrder) {
    controller.getEventBus.publishCold(MyFinishedEvent(
      e.getKey, order.getState,
      Option(emptyToNull(order.getParameters.get(SpoolerProcessAfterNames.parameter))) map { _.toBoolean }))
  }

  @EventHandler def handleEvent(e: LogEvent) {
    if (List(LogLevel.error, LogLevel.warning) contains e.level) {
      e.getCodeOrNull match {
        case code: String => messageCodes.addBinding(e.level, code)
        case _ =>
      }
    }
  }
}

object SpoolerProcessAfterTest {
  private val logger = LoggerFactory.getLogger(classOf[SpoolerProcessAfterTest])

  private case class MyFinishedEvent(orderKey: OrderKey, state: OrderState, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event
}
