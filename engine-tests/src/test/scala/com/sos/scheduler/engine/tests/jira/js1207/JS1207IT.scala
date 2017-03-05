package com.sos.scheduler.engine.tests.jira.js1207

import com.sos.jobscheduler.common.scalautil.Closers._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.event.KeyedEvent.NoKey
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderId, OrderNestedFinished, OrderNestedStarted, OrderStarted, OrderStepEnded, OrderStepStarted}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.EventHandlerFailedEvent
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
   * @return Map with maxima of simulateneously started orders per jobchain
   */
  private def runOrders(outerJobchainPath: JobChainPath, jobchainLimits: Map[JobChainPath, Int], n: Int): Map[JobChainPath, Int] =
    withCloser { implicit closer ⇒
      val promise = Promise[Unit]()
      var promisedFinishedOrderCount = n
      val counters = mutable.Map[JobChainPath, Statistic]() ++ (jobchainLimits map { case (path, limit) ⇒ path → new Statistic(limit) })
      eventBus.on[OrderStarted.type] { case KeyedEvent(orderKey, _) ⇒
        orderKey.jobChainPath shouldEqual AInnerJobChainPath // The first inner jobchain, not OutJobChainPath as one may expect
        counters(outerJobchainPath).onStarted()
      }
      eventBus.on[OrderFinished] { case KeyedEvent(orderKey, _) ⇒
        orderKey.jobChainPath shouldEqual CInnerJobChainPath // The last inner jobchain, not OutJobChainPath as one may expect
        counters(outerJobchainPath).onFinished()
        promisedFinishedOrderCount -= 1
        if (promisedFinishedOrderCount == 0) promise.success(())
      }
      eventBus.on[OrderNestedStarted.type] { case KeyedEvent(orderKey, _)  ⇒
        counters(orderKey.jobChainPath).onStarted()
      }
      eventBus.on[OrderNestedFinished.type] { case KeyedEvent(orderKey, _) ⇒
        counters(orderKey.jobChainPath).onFinished()
      }
      eventBus.on[OrderStepStarted] { case KeyedEvent(orderKey, _) ⇒
        counters(outerJobchainPath).onStepStarted()
        counters(orderKey.jobChainPath).onStepStarted()
      }
      eventBus.on[OrderStepEnded] { case KeyedEvent(orderKey, _) ⇒
        counters(orderKey.jobChainPath).onStepEnded()
        counters(outerJobchainPath).onStepEnded()
      }
      eventBus.on[EventHandlerFailedEvent] { case KeyedEvent(NoKey, e) ⇒
        promise.tryFailure(e.throwable)
      }
      inSchedulerThread {
        // Run as single batch for immediate processing
        for (i ← 1 to n) scheduler executeXml OrderCommand(outerJobchainPath orderKey OrderId(s"TEST-ORDER-$i"))
      }
      awaitSuccess(promise.future)
      for ((jobchainPath, statistics) ← counters) withClue(s"$jobchainPath: ") {
        statistics.started shouldEqual 0
        statistics.inStep shouldEqual 0
      }
      counters.toMap collect { case (path, statistic) if statistic.startedMaximum != 0 ⇒ path → statistic.startedMaximum }
    }
}

private object JS1207IT {
  private val UnlimitedOuterJobChainPath = JobChainPath("/test-outer-unlimited")
  private val LimitedOuterJobChainPath = JobChainPath("/test-outer-limited")
  private val AInnerJobChainPath = JobChainPath("/test-inner-a")
  private val BInnerJobChainPath = JobChainPath("/test-inner-b")
  private val CInnerJobChainPath = JobChainPath("/test-inner-c")

  private class Statistic(limit: Int) {
    var started = 0
    var startedMaximum = 0
    var inStep = 0
    var inStepMaximum = 0

    assertInvariant()

    def onStarted(): Unit = {
      started += 1
      if (startedMaximum < started) {
        startedMaximum = started
      }
      assertInvariant()
    }

    def onFinished(): Unit = {
      started -= 1
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
      assert(inStep <= started)
      assert(inStep <= startedMaximum)
      assert(inStep <= limit)
      assert(inStepMaximum <= limit)
      assert(started <= startedMaximum)
      assert(started <= limit, s"Number of started orders ($startedMaximum) exceeds limit ($limit)")
      assert(startedMaximum <= limit, s"Maximum number of started orders ($startedMaximum) exceeds limit ($limit)")
    }
  }
}
