package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{TaskId, TaskClosedEvent}
import com.sos.scheduler.engine.data.log.{LogEvent, LogLevel}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{EventHandler, HotEventHandler}
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, UnmodifiableOrder}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class SpoolerProcessAfterTest extends ScalaSchedulerTest {

  import SpoolerProcessAfterTest._

  private lazy val jobSubsystem = scheduler.instance[JobSubsystem]
  private lazy val orderSubsystem = scheduler.instance[OrderSubsystem]
  private val messageCodes = new MyMutableMultiMap[LogLevel, String]

  controller.setTerminateOnError(false)

  Settings.list.zipWithIndex foreach { case ((setting, expected), i) =>
    val index = i + 1
    test(index +". "+ setting +" should result in " +expected) {
      new MyTest(index, setting, expected)
    }
  }

  private class MyTest(index: Int, setting: Setting, expected: Expected) {
    val eventPipe = controller.newEventPipe()
    val job = jobSubsystem.job(setting.jobPath)

    try checkAssertions(execute())
    finally cleanUp(setting)

    def execute() = {
      scheduler executeXml setting.orderElem
      val myFinishedEvent = eventPipe.next[MyFinishedEvent]
      orderSubsystem.tryRemoveOrder(setting.orderKey)  // Falls Auftrag zurückgestellt ist, damit der Job nicht gleich nochmal mit demselben Auftrag startet.
      job.endTasks()   // Job ist möglicherweise schon gestoppt
      assert(eventPipe.next[TaskClosedEvent].getId === new TaskId(index), "TaskClosedEvent not for expected task - probably a previous test failed")
      myFinishedEvent
    }

    def checkAssertions(event: MyFinishedEvent) {
      assert(event.orderKey === setting.orderKey)
      assert(expected.orderStateExpectation matches event.state, "Expected OrderState="+expected.orderStateExpectation+", but was "+event.state)
      assert(event.spoolerProcessAfterParameterOption === expected.spoolerProcessAfterParameterOption, "Parameter for spooler_process_after()")
      assert(job.state === expected.jobState, "Job.state is not as expected")
      assert(messageCodes.toMap === expected.messageCodes.toMap)
    }

    private def cleanUp(setting: Setting) {
      scheduler executeXml <modify_job job={setting.jobPath.asString} cmd="unstop"/>
      messageCodes.clear()
    }
  }

  @HotEventHandler def handleEvent(e: OrderStepEndedEvent, order: UnmodifiableOrder) {
    if (e.stateTransition == OrderStateTransition.keepState) {
      // Es wird kein OrderFinishedEvent geben.
      publishMyFinishedEvent(order)
    }
  }

  @HotEventHandler def handleEvent(e: OrderFinishedEvent, order: UnmodifiableOrder) {
    publishMyFinishedEvent(order)
  }

  private def publishMyFinishedEvent(order: UnmodifiableOrder) {
    controller.getEventBus.publishCold(MyFinishedEvent(
      order.getKey, order.getState,
      Option(emptyToNull(order.getParameters.get(SpoolerProcessAfterNames.parameter))) map { _.toBoolean }))
  }

  @EventHandler def handleEvent(e: LogEvent) {
    if (Expected.logLevels contains e.level) {
      e.getCodeOrNull match {
        case code: String => messageCodes.addBinding(e.level, code)
        case null =>
      }
    }
  }
}

object SpoolerProcessAfterTest {
  //private val logger = LoggerFactory.getLogger(classOf[SpoolerProcessAfterTest])

  private class MyMutableMultiMap[A,B] extends mutable.HashMap[A, mutable.Set[B]] with mutable.MultiMap[A, B]

  private case class MyFinishedEvent(orderKey: OrderKey, state: OrderState, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event
}
