package com.sos.scheduler.engine.tests.jira.js1207

import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderId, OrderNestedFinishedEvent, OrderNestedTouchedEvent, OrderStepEndedEvent, OrderStepStartedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.EventHandlerFailedEvent
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.test.SchedulerTestUtils.awaitSuccess
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1207.JS1207IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.concurrent.Promise

/**
 * JS-1198 und JS-1207 max_orders in nested jobchains.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1207IT extends FreeSpec with ScalaSchedulerTest {

  private implicit lazy val schedulerThreadCallQueue = instance[SchedulerThreadCallQueue]

  "JS-1198 limited inner jobchains" in {
    val expectedMaxima = Map(
      UnlimitedOuterJobChainPath → 7,   // Einer mehr als 3+2+1, weil Aufträge zwischen zwei inneren Jobketten hängen können
      AInnerJobChainPath → 3,
      BInnerJobChainPath → 2,
      CInnerJobChainPath → 1
    )
    runOrders(UnlimitedOuterJobChainPath, expectedMaxima, n = 7) shouldEqual expectedMaxima
  }

  "JS-1207 limited outer and inner jobchains" in {
    val expectedMaxima = Map(
      LimitedOuterJobChainPath → 2,
      AInnerJobChainPath → 2,
      BInnerJobChainPath → 2,
      CInnerJobChainPath → 1
    )
    runOrders(LimitedOuterJobChainPath, expectedMaxima, n = 7) shouldEqual expectedMaxima
  }

  /**
   * @return Map with maxima of simulateneously running orders per jobchain
   */
  private def runOrders(outerJobchainPath: JobChainPath, jobchainLimits: Map[JobChainPath, Int], n: Int): Map[JobChainPath, Int] =
    autoClosing(Closer.create()) { implicit closer ⇒
      val promise = Promise[Unit]()
      var promisedFinishedOrderCount = n
      val counters = mutable.Map[JobChainPath, Statistic]() ++ (jobchainLimits map { case (path, limit) ⇒ path → new Statistic(limit) })
      eventBus.on[OrderTouchedEvent] { case e ⇒
        e.orderKey.jobChainPath shouldEqual AInnerJobChainPath // The first inner jobchain, not OutJobChainPath as one may expect
        counters(outerJobchainPath).onStarted()
      }
      eventBus.on[OrderFinishedEvent] { case e ⇒
        e.orderKey.jobChainPath shouldEqual CInnerJobChainPath // The last inner jobchain, not OutJobChainPath as one may expect
        counters(outerJobchainPath).onFinished()
        promisedFinishedOrderCount -= 1
        if (promisedFinishedOrderCount == 0) promise.success(())
      }
      eventBus.on[OrderNestedTouchedEvent] { case e ⇒
        counters(e.orderKey.jobChainPath).onStarted()
      }
      eventBus.on[OrderNestedFinishedEvent] { case e ⇒
        counters(e.orderKey.jobChainPath).onFinished()
      }
      eventBus.on[OrderStepStartedEvent] { case e ⇒
        counters(outerJobchainPath).onStepStarted()
        counters(e.orderKey.jobChainPath).onStepStarted()
      }
      eventBus.on[OrderStepEndedEvent] { case e ⇒
        counters(e.orderKey.jobChainPath).onStepEnded()
        counters(outerJobchainPath).onStepEnded()
      }
      eventBus.on[EventHandlerFailedEvent] { case e ⇒
        promise.tryFailure(e.getThrowable)
      }
      inSchedulerThread {
        // Run as single batch for immediate processing
        for (i ← 1 to n) scheduler executeXml OrderCommand(outerJobchainPath orderKey OrderId(s"TEST-ORDER-$i"))
      }
      awaitSuccess(promise.future)
      for ((jobchainPath, statistics) ← counters) withClue(s"$jobchainPath: ") {
        statistics.running shouldEqual 0
        statistics.inStep shouldEqual 0
      }
      counters.toMap collect { case (path, statistic) if statistic.runningMaximum != 0 ⇒ path → statistic.runningMaximum }
    }

  private def eventBus = controller.getEventBus
}

private object JS1207IT {
  private val UnlimitedOuterJobChainPath = JobChainPath("/test-outer-unlimited")
  private val LimitedOuterJobChainPath = JobChainPath("/test-outer-limited")
  private val AInnerJobChainPath = JobChainPath("/test-inner-a")
  private val BInnerJobChainPath = JobChainPath("/test-inner-b")
  private val CInnerJobChainPath = JobChainPath("/test-inner-c")

  private class Statistic(limit: Int) {
    var running = 0
    var runningMaximum = 0
    var inStep = 0
    var inStepMaximum = 0

    assertInvariant()

    def onStarted(): Unit = {
      running += 1
      if (runningMaximum < running) {
        runningMaximum = running
      }
      assertInvariant()
    }

    def onFinished(): Unit = {
      running -= 1
      assertInvariant()
    }

    def onStepStarted(): Unit = {
      inStep += 1
      if (inStepMaximum < inStep) {
        inStepMaximum = inStep
      }
      assertInvariant()
    }

    def onStepEnded(): Unit = {
      inStep -= 1
      assertInvariant()
    }

    def assertInvariant(): Unit = {
      assert(inStep <= running)
      assert(inStep <= runningMaximum)
      assert(inStep <= limit)
      assert(inStepMaximum <= limit)
      assert(running <= runningMaximum)
      assert(running <= limit, s"Number of running orders ($runningMaximum) exceeds limit ($limit)")
      assert(runningMaximum <= limit, s"Maximum number of running orders ($runningMaximum) exceeds limit ($limit)")
    }
  }
}
