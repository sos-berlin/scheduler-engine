package com.sos.scheduler.engine.tests.jira.js1464

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, _}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.DatabaseOrderCheckPeriod
import com.sos.scheduler.engine.kernel.settings.CppSettingName.useOldMicroschedulingForJobs
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1464.JS1464IT._
import java.time.Duration
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1464 Order's job chain determines a tasks's process class.
 * <p>
 * &lt;job_chain process_class="..."/>
 * <p>
 * @see JS-1301, JS1301IT
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1464IT extends FreeSpec with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"),
    cppSettings = Map(useOldMicroschedulingForJobs → false.toString))

  "A job with two task can simultaneously run two orders in different process classes" - {
    testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
      val aOrderKey = aJobChainPath orderKey "1"
      val bOrderKey = bJobChainPath orderKey "1"
      eventBus.awaitingKeyedEvent[OrderTouchedEvent](bOrderKey) {
        eventBus.awaitingKeyedEvent[OrderTouchedEvent](aOrderKey) {
          addOrder(aOrderKey, 1.s)
          addOrder(bOrderKey, 1.s)
        }
      }
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](bOrderKey) {
        eventBus.awaitingKeyedEvent[OrderFinishedEvent](aOrderKey) {
        } .state shouldBe EndState
      } .state shouldBe EndState
    }
  }

  "A third order with a third process class is postponed until a new task can be started" - {
    testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
      val a1OrderKey = aJobChainPath orderKey "1"
      val a2OrderKey = aJobChainPath orderKey "2"
      val bOrderKey  = bJobChainPath orderKey "1"
      eventBus.awaitingKeyedEvent[OrderTouchedEvent](bOrderKey) {
        eventBus.awaitingKeyedEvent[OrderTouchedEvent](a1OrderKey) {
          addOrder(a1OrderKey, 2.s)
          addOrder(bOrderKey, 4.s)
        }
      }
      autoClosing(controller.newEventPipe()) { eventPipe ⇒
        addOrder(a2OrderKey, 1.s)
        eventPipe.nextKeyed[OrderFinishedEvent](a1OrderKey).state shouldBe EndState
        eventPipe.nextKeyed[OrderTouchedEvent](a2OrderKey)
        expectEndStateReached(eventPipe, a2OrderKey, bOrderKey)
      }
    }
  }

  "A third order is postponed until a new task can be started" - {
    testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
      val aOrderKey = aJobChainPath orderKey "1"
      val bOrderKey = bJobChainPath orderKey "1"
      val cOrderKey = cJobChainPath orderKey "1"
      eventBus.awaitingKeyedEvent[OrderTouchedEvent](bOrderKey) {
        eventBus.awaitingKeyedEvent[OrderTouchedEvent](aOrderKey) {
          addOrder(aOrderKey, 2.s)
          addOrder(bOrderKey, 5.s + (if (isDistributed) DatabaseOrderCheckPeriod else 0.s))
        }
      }
      autoClosing(controller.newEventPipe()) { eventPipe ⇒
        addOrder(cOrderKey, 0.s)
        eventPipe.nextKeyed[OrderFinishedEvent](aOrderKey).state shouldBe EndState
        eventPipe.nextWithCondition[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-271"))
        eventPipe.nextKeyed[OrderTouchedEvent](cOrderKey)
        expectEndStateReached(eventPipe, bOrderKey, cOrderKey)
      }
    }
  }

  private def testNonDistributedAndDistributed(body: (Boolean, JobChainPath, JobChainPath, JobChainPath) ⇒ Unit): Unit =
    for ((isDistributed, testName) ← List(false → "not distributed", true → "distributed")) testName in {
      val suffix = if (isDistributed) "-distributed" else ""
      body(isDistributed, JobChainPath(s"/test-a$suffix"), JobChainPath(s"/test-b$suffix"), JobChainPath(s"/test-c$suffix"))
    }

  private def addOrder(orderKey: OrderKey, sleep: Duration): Unit = {
    scheduler executeXml OrderCommand(orderKey, parameters = Map("sleep" → sleep.toMillis.toString))
  }

  private def expectEndStateReached(eventPipe: EventPipe, orderKeys: OrderKey*): Unit =
    assertResult((orderKeys map { _ → EndState }).toSet) {
      (eventPipe.nextKeyedEvents[OrderFinishedEvent](orderKeys) map { e ⇒ e.key → e.state }).toSet
    }
}

private object JS1464IT {
  private val EndState = OrderState("END")
}
