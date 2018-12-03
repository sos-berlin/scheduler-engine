package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.async.{CallRunner, StandardCallQueue}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelectorTest._
import com.sos.scheduler.engine.kernel.processclass.common.selection.{FixedPriority, RoundRobin, SelectionMethod}
import java.time.Instant
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import scala.concurrent.Future
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FailableSelectorTest extends FreeSpec {

  "RoundRobin" - {
    val context = new Context(RoundRobin)
    import context.{_now, callRunner, callbacks, implicitExecutionContext, newFailableSelector, runSelector}

    "Boths processors are accessible" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      when(callbacks.apply(bFailable)) thenReturn Future { Success(yResult) }
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(1)).apply(aFailable)
      verify(callbacks, times(1)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "One processor is not accessible" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Failure { new InaccessibleException } }
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(2)).apply(aFailable)
      verify(callbacks, times(3)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Both processors are accessible again, but delay as not elapsed" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(2)).apply(aFailable)
      verify(callbacks, times(5)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Delay has elapsed" in {
      _now += TestDelay
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(3)).apply(aFailable)
      verify(callbacks, times(6)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Both processor are inaccessible, then one is accessible again" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Failure { new InaccessibleException } }
      when(callbacks.apply(bFailable)) thenReturn Future { Failure { new InaccessibleException } }
      val pendingFuture = runSelector()
      pendingFuture.value shouldEqual None
      verify(callbacks, times(4)).apply(aFailable)
      verify(callbacks, times(7)).apply(bFailable)
      verify(callbacks, times(1)).onDelay(TestDelay, aFailable)
      verifyNoMoreInteractions(callbacks)

      _now += TestDelay
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      callRunner.executeMatureCalls()
      pendingFuture.value shouldEqual Some(Success(aFailable → Success(xResult)))
      verify(callbacks, times(5)).apply(aFailable)
      verify(callbacks, times(7)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Other failure aborts FailableSelector" in {
      when(callbacks.apply(bFailable)) thenReturn Future { throw BadException }
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Failure(BadException)))
      verify(callbacks, times(6)).apply(aFailable)
      verify(callbacks, times(8)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "One more failure" in {
      when(callbacks.apply(aFailable)) thenReturn Future { throw BadException }
      runSelector().value shouldEqual Some(Success(aFailable → Failure(BadException)))
      verify(callbacks, times(7)).apply(aFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "cancel 1" in {
      val selector = newFailableSelector()
      val future = selector.start()
      selector.cancel()
      callRunner.executeMatureCalls()
      future.value.get.get._2.failed.get shouldBe a [FailableSelector.CancelledException]
    }

    "cancel 2" in {
      val selector = newFailableSelector()
      val future = selector.start()
      callRunner.executeMatureCalls()
      selector.cancel()
      callRunner.executeMatureCalls()
      future.value.get.get._2.failed.get shouldBe a [FailableSelector.CancelledException]
    }
  }

  "FixedPriority" - {
    val context = new Context(FixedPriority)
    import context.{_now, callRunner, callbacks, implicitExecutionContext, newFailableSelector, runSelector}

    "Boths processors are accessible" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      when(callbacks.apply(bFailable)) thenReturn Future { Success(yResult) }
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      verify(callbacks, times(2)).apply(aFailable)
      verify(callbacks, times(0)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "One processor is not accessible" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Failure { new InaccessibleException } }
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(2 + 1)).apply(aFailable)
      verify(callbacks, times(0 + 2)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Both processors are accessible again, but delay as not elapsed" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      runSelector().value shouldEqual Some(Success(bFailable → Success(yResult)))
      verify(callbacks, times(2 + 1 + 0)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Delay has elapsed" in {
      _now += TestDelay
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      verify(callbacks, times(2 + 1 + 0 + 2)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Both processor are inaccessible, then one is accessible again" in {
      when(callbacks.apply(aFailable)) thenReturn Future { Failure { new InaccessibleException } }
      when(callbacks.apply(bFailable)) thenReturn Future { Failure { new InaccessibleException } }
      val pendingFuture = runSelector()
      pendingFuture.value shouldEqual None
      verify(callbacks, times(2 + 1 + 0 + 2 + 1)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0 + 1)).apply(bFailable)
      verify(callbacks, times(1)).onDelay(TestDelay, aFailable)
      verifyNoMoreInteractions(callbacks)

      _now += TestDelay
      when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
      callRunner.executeMatureCalls()
      pendingFuture.value shouldEqual Some(Success(aFailable → Success(xResult)))
      verify(callbacks, times(2 + 1 + 0 + 2 + 1 + 1)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0 + 1 + 0)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Failure in b ignored while staying in a" in {
      when(callbacks.apply(bFailable)) thenReturn Future { throw BadException }
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      runSelector().value shouldEqual Some(Success(aFailable → Success(xResult)))
      verify(callbacks, times(2 + 1 + 0 + 2 + 1 + 1 + 2)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0 + 1 + 0 + 0)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "Other failure aborts FailableSelector" in {
      when(callbacks.apply(aFailable)) thenReturn Future { throw BadException }
      runSelector().value shouldEqual Some(Success(aFailable → Failure(BadException)))
      verify(callbacks, times(2 + 1 + 0 + 2 + 1 + 1 + 2 + 1)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0 + 1 + 0 + 0 + 0)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "One more failure" in {
      when(callbacks.apply(bFailable)) thenReturn Future { throw BadException }
      runSelector().value shouldEqual Some(Success(bFailable → Failure(BadException)))
      verify(callbacks, times(2 + 1 + 0 + 2 + 1 + 1 + 2 + 1 + 0)).apply(aFailable)
      verify(callbacks, times(0 + 2 + 2 + 0 + 1 + 0 + 0 + 0 + 1)).apply(bFailable)
      verifyNoMoreInteractions(callbacks)
    }

    "cancel 1" in {
      val selector = newFailableSelector()
      val future = selector.start()
      selector.cancel()
      callRunner.executeMatureCalls()
      future.value.get.get._2.failed.get shouldBe a [FailableSelector.CancelledException]
    }

    "cancel 2" in {
      val selector = newFailableSelector()
      val future = selector.start()
      callRunner.executeMatureCalls()
      selector.cancel()
      callRunner.executeMatureCalls()
      future.value.get.get._2.failed.get shouldBe a [FailableSelector.CancelledException]
    }
  }
}

private object FailableSelectorTest {
  private val TestDelay = 60.s
  private val aFailable = Failable("A")
  private val bFailable = Failable("B")
  private val Failables = List(aFailable, bFailable)
  private val xResult = Result("X")
  private val yResult = Result("Y")
  private val BadException = new Exception

  private case class Failable(id: String)
  private case class Result(string: String)
  private class InaccessibleException extends Exception

  private class Context(selectionMethod: SelectionMethod) {
    var _now = Instant.ofEpochSecond(10 * 24 * 3600)  // Some instant
    private val failables = new FailableCollection[Failable](Failables, () ⇒ TestDelay, selectionMethod) {
      override def now = _now
    }
    private val callQueue = new StandardCallQueue {
      override def currentTimeMillis = _now.toEpochMilli
    }
    implicit val implicitExecutionContext = callQueue.implicits.executionContext
    val callRunner = new CallRunner(callQueue)
    val callbacks = mock[FailableSelector.Callbacks[Failable, Result]]

    def runSelector(): Future[(Failable, Try[Result])] = {
      val future = newFailableSelector().start()
      callRunner.executeMatureCalls()
      future
    }

    def newFailableSelector() =
      new FailableSelector(failables, callbacks, callQueue, connectionTimeout = None) {
        override def now = _now
      }
    }
}
