package com.sos.scheduler.engine.tests.jira.js1464

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.InfoLogged
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey, _}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.DatabaseOrderCheckPeriod
import com.sos.scheduler.engine.kernel.settings.CppSettingName.useOldMicroschedulingForJobs
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.SchedulerTestUtils._
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

  "Job task limit is respected" - {
    "A job with two task can simultaneously run two orders in different process classes" - {
      testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
        val aOrderKey = aJobChainPath orderKey "1"
        val bOrderKey = bJobChainPath orderKey "1"
        eventBus.awaiting[OrderStarted.type](bOrderKey) {
          eventBus.awaiting[OrderStarted.type](aOrderKey) {
            addOrder(aOrderKey, 1.s)
            addOrder(bOrderKey, 1.s)
          }
        }
        eventBus.awaiting[OrderFinished](bOrderKey) {
          eventBus.awaiting[OrderFinished](aOrderKey) {
          } .nodeId shouldBe EndNodeId
        } .nodeId shouldBe EndNodeId
      }
    }

    "A third order with same process class as the first order is postponed until a new task can be started" - {
      testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
        val a1OrderKey = aJobChainPath orderKey "1"
        val a2OrderKey = aJobChainPath orderKey "2"
        val bOrderKey  = bJobChainPath orderKey "1"
        eventBus.awaiting[OrderStarted.type](bOrderKey) {
          eventBus.awaiting[OrderStarted.type](a1OrderKey) {
            addOrder(a1OrderKey, 2.s)
            addOrder(bOrderKey, 4.s)
          }
        }
        withEventPipe { eventPipe ⇒
          addOrder(a2OrderKey, 1.s)
          eventPipe.next[OrderFinished](a1OrderKey).nodeId shouldBe EndNodeId
          eventPipe.next[OrderStarted.type](a2OrderKey)
          expectEndStateReached(eventPipe, a2OrderKey, bOrderKey)
        }
      }
    }

    "A third order with a third process class is postponed until a new task can be started" - {
      testNonDistributedAndDistributed { (isDistributed, aJobChainPath, bJobChainPath, cJobChainPath) ⇒
        val aOrderKey = aJobChainPath orderKey "1"
        val bOrderKey = bJobChainPath orderKey "1"
        val cOrderKey = cJobChainPath orderKey "1"
        eventBus.awaiting[OrderStarted.type](bOrderKey) {
          eventBus.awaiting[OrderStarted.type](aOrderKey) {
            addOrder(aOrderKey, 2.s)
            addOrder(bOrderKey, 5.s + (if (isDistributed) DatabaseOrderCheckPeriod else 0.s))
          }
        }
        withEventPipe { eventPipe ⇒
          addOrder(cOrderKey, 0.s)
          eventPipe.next[OrderFinished](aOrderKey).nodeId shouldBe EndNodeId
          eventPipe.nextWhen[InfoLogged](_.event.codeOption contains MessageCode("SCHEDULER-271"))
          eventPipe.next[OrderStarted.type](cOrderKey)
          expectEndStateReached(eventPipe, bOrderKey, cOrderKey)
        }
      }
    }
  }

  private def testNonDistributedAndDistributed(body: (Boolean, JobChainPath, JobChainPath, JobChainPath) ⇒ Unit): Unit =
    for ((isDistributed, testName) ← List(false → "not distributed", true → "distributed")) testName in {
      val suffix = if (isDistributed) "-distributed" else ""
      body(isDistributed, JobChainPath(s"/test-a$suffix"), JobChainPath(s"/test-b$suffix"), JobChainPath(s"/test-c$suffix"))
    }

  "Task waits when process class limit is reached and tries to end idling tasks (JS-1481)" in {
    val aJobChainPath = JobChainPath("/test-one-a")
    val bJobChainPath = JobChainPath("/test-one-b")
    val a1OrderKey = aJobChainPath orderKey "1"
    val b1OrderKey = bJobChainPath orderKey "1"
    val a2OrderKey = aJobChainPath orderKey "2"
    val b2OrderKey = bJobChainPath orderKey "2"
    withEventPipe { eventPipe ⇒
      addOrder(a1OrderKey, 2.s)
      addOrder(b1OrderKey, 2.s)
      eventPipe.next[OrderStepStarted](Set(a1OrderKey, b1OrderKey))
      eventBus.awaitingWhen[InfoLogged](_.event.codeOption contains MessageCode("SCHEDULER-271")) {   // "Task is being terminated in favour of ..."
        eventBus.awaiting[OrderFinished](a1OrderKey) {
          eventBus.awaiting[OrderFinished](b1OrderKey) {
            addOrder(a2OrderKey, 1.s)
            addOrder(b2OrderKey, 1.s)
          }
        }
      }
      eventPipe.queued[OrderStepStarted] shouldBe empty
      val eventPipe2 = controller.newEventPipe()
      eventPipe.next[OrderFinished](Set(a2OrderKey, b2OrderKey))
      eventPipe2.queued[OrderStepStarted] filter { e ⇒ Set(a2OrderKey, b2OrderKey)(e.key) } map { _.event.asInstanceOf[OrderStepStarted].nodeId } shouldEqual
        List(NodeId("100"), NodeId("100"))
    }
  }

  "Handling multiple tasks in a limited process class (JS-1516)" in {
    val aJobPath = JobPath("/js1516/test-a")
    val bJobPath = JobPath("/js1516/test-b")
    val futures = inSchedulerThread {
      for (jobPath ← List(aJobPath, bJobPath, aJobPath, bJobPath)) yield startJob(jobPath).result
    }
    awaitResults(futures)
  }

  private def addOrder(orderKey: OrderKey, sleep: Duration): Unit = {
    scheduler executeXml OrderCommand(orderKey, parameters = Map("sleep" → sleep.toMillis.toString))
  }

  private def expectEndStateReached(eventPipe: EventPipe, orderKeys: OrderKey*): Unit =
    assertResult((orderKeys map { _ → EndNodeId }).toSet) {
      (eventPipe.next[OrderFinished](orderKeys) map { e ⇒ e.key → e.event.asInstanceOf[OrderFinished].nodeId }).toSet
    }
}

private object JS1464IT {
  private val EndNodeId = NodeId("END")
}
