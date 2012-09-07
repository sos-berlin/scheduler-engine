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
import com.sos.scheduler.engine.test.util.WaitFor.waitFor
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.joda.time.Duration.{millis, standardSeconds}
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
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
    test(renameTestForSurefire(index +". "+ setting +" should result in " +expected)) {
      new MyTest(index, setting, expected)
    }
  }

  private final class MyTest(index: Int, setting: Setting, expected: Expected) {
    val eventPipe = controller.newEventPipe()
    val job = jobSubsystem.job(setting.jobPath)

    try {
      val e = execute()
      checkAssertions(e)
    }
    finally cleanUp()

    def execute() = {
      scheduler executeXml setting.orderElem
      val result = eventPipe.next[MyFinishedEvent]
      orderSubsystem.tryRemoveOrder(setting.orderKey)  // Falls Auftrag zurückgestellt ist, damit der Job nicht gleich nochmal mit demselben Auftrag startet.
      job.endTasks()   // Job ist möglicherweise schon gestoppt
      val e = eventPipe.next[TaskClosedEvent]
      assert(e.getId === new TaskId(index), "TaskClosedEvent not for expected task - probably a previous test failed")
      waitFor(timeout=standardSeconds(5), step=millis(10)) { job.state == expected.jobState }   // Der Job-Zustand wird asynchron geändert (stopping -> stopped, running -> pending). Wir warten kurz darauf.
      result
    }

    def checkAssertions(event: MyFinishedEvent) {
      assert(event.orderKey === setting.orderKey)
      assert(expected.orderStateExpectation matches event.state, "Expected OrderState="+expected.orderStateExpectation+", but was "+event.state)
      assert(event.spoolerProcessAfterParameterOption === expected.spoolerProcessAfterParameterOption, "Parameter for spooler_process_after(): ")
      assert(job.state === expected.jobState, "Job.state is not as expected: ")
      assert(messageCodes.toMap === expected.messageCodes.toMap)
    }

    private def cleanUp() {
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
      for (code <- Option(e.getCodeOrNull))
        messageCodes.addBinding(e.level, code)
    }
  }
}

private object SpoolerProcessAfterTest {
  //private val logger = LoggerFactory.getLogger(classOf[SpoolerProcessAfterTest])

  class MyMutableMultiMap[A,B] extends mutable.HashMap[A, mutable.Set[B]] with mutable.MultiMap[A, B]

  case class MyFinishedEvent(orderKey: OrderKey, state: OrderState, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event

  /** Wegen JUnitRunner? Klammern lassen Surefire Klassen- und Testnamen durcheinanderbringen */
  def renameTestForSurefire(name: String) = name.replace('(', '[').replace(')', ']')
      .replace(',', ' ')  // Surefire zeigt Komma als \u002C
}
