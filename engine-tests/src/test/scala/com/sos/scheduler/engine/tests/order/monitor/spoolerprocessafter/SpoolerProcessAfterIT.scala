package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.log.{LogEvent, SchedulerLogLevel}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.EventSourceEvent
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, UnmodifiableOrder}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.taskIdOffset
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.time.TimeoutWithSteps
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.SpoolerProcessAfterIT._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.joda.time.Duration.millis
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class SpoolerProcessAfterIT extends FunSuite with ScalaSchedulerTest {

  import controller.eventBus

  private val messageCodes = new MyMutableMultiMap[SchedulerLogLevel, String]

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)

  private lazy val jobSubsystem = instance[JobSubsystem]
  private lazy val orderSubsystem = instance[OrderSubsystem]

  Settings.list.zipWithIndex foreach { case ((setting, expected), i) ⇒
    val index = i + 1
    test(renameTestForSurefire(s"$index. $setting should result in $expected")) {
      myTest(index, setting, expected)
    }
  }

  private def myTest(index: Int, setting: Setting, expected: Expected): Unit =
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      val job = jobSubsystem.job(setting.jobPath)

      try {
        val e = execute()
        checkAssertions(e)
      }
      finally cleanUpAfterTest()

      def execute() = {
        scheduler executeXml setting.orderElem
        val result = eventPipe.nextAny[MyFinishedEvent]
        cleanUpAfterExcecute()
        result
      }

      def cleanUpAfterExcecute(): Unit = {
        orderSubsystem.tryRemoveOrder(setting.orderKey)  // Falls Auftrag zurückgestellt ist, damit der Job nicht gleich nochmal mit demselben Auftrag startet.
        job.endTasks()   // Task kann schon beendet und Job schon gestoppt sein.
        eventPipe.nextAny[TaskClosedEvent] match { case e ⇒
          assert(e.taskId == TaskId(taskIdOffset + index - 1), "TaskClosedEvent not for expected task - probably a previous test failed")
        }
        waitForCondition(TimeoutWithSteps(millis(3000), millis(10))) { job.state == expected.jobState }   // Der Job-Zustand wird asynchron geändert (stopping -> stopped, running -> pending). Wir warten kurz darauf.
      }

      def checkAssertions(event: MyFinishedEvent): Unit = {
        assert(event.orderKey == setting.orderKey)
        assert(expected.orderStateExpectation matches event.state, s", expected OrderState=${expected.orderStateExpectation}, but was ${event.state}")
        assert(event.spoolerProcessAfterParameterOption == expected.spoolerProcessAfterParameterOption, "Parameter for spooler_process_after(): ")
        assert(job.state == expected.jobState, ", Job.state is not as expected")
        assert(messageCodes.toMap == expected.messageCodes.toMap)
      }

      def cleanUpAfterTest(): Unit = {
        scheduler executeXml <modify_job job={setting.jobPath.string} cmd="unstop"/>
        messageCodes.clear()
      }
    }

  eventBus.onHotEventSourceEvent[OrderStepEndedEvent] {
    case EventSourceEvent(e: OrderStepEndedEvent, order: UnmodifiableOrder) ⇒
      if (e.stateTransition == KeepOrderStateTransition) {
        // Es wird kein OrderFinishedEvent geben.
        publishMyFinishedEvent(order)
      }
  }

  eventBus.onHotEventSourceEvent[OrderFinishedEvent] {
    case EventSourceEvent(e: OrderFinishedEvent, order: UnmodifiableOrder) ⇒
    publishMyFinishedEvent(order)
  }

  private def publishMyFinishedEvent(order: UnmodifiableOrder): Unit = {
    controller.eventBus.publishCold(MyFinishedEvent(
      order.key, order.state,
      emptyToNone(order.parameters(SpoolerProcessAfterNames.parameter)) map { _.toBoolean }))
  }

  eventBus.on[LogEvent] { case e: LogEvent ⇒
    if (Expected.LogLevels contains e.level) {
      for (code ← Option(e.getCodeOrNull))
        messageCodes.addBinding(e.level, code)
    }
  }
}


private object SpoolerProcessAfterIT {
  private class MyMutableMultiMap[A,B] extends mutable.HashMap[A, mutable.Set[B]] with mutable.MultiMap[A, B]

  private case class MyFinishedEvent(orderKey: OrderKey, state: OrderState, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event

  /** Wegen JUnitRunner? Klammern lassen Surefire Klassen- und Testnamen durcheinanderbringen */
  private def renameTestForSurefire(name: String) =
    name.replace('(', '[').replace(')', ']')
      .replace(',', ' ')  // Surefire zeigt Komma als \u002C
}
