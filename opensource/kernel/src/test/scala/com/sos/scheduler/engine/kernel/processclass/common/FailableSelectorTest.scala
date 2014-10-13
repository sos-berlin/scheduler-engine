package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.async.{CallRunner, StandardCallQueue}
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelectorTest._
import org.joda.time.Instant
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import scala.concurrent.Future
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FailableSelectorTest extends FreeSpec {

  private var _now = new Instant(10 * 24 * 3600 * 1000)   // Some instant
  private val failables = new FailableCollection[Failable](Failables, TestDelay) {
    override def now = _now
  }
  private val callQueue = new StandardCallQueue {
    override def currentTimeMillis = _now.getMillis
  }

  import callQueue.implicits.executionContext

  private val callRunner = new CallRunner(callQueue)
  private val callbacks = mock[FailableSelector.Callbacks[Failable, Result]]

  "Boths processors are accessible" in {
    when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
    when(callbacks.apply(bFailable)) thenReturn Future { Success(yResult) }
    runSelector().value shouldEqual Some(Success(aFailable → xResult))
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
    verify(callbacks, times(1)).apply(aFailable)
    verify(callbacks, times(1)).apply(bFailable)
    verifyNoMoreInteractions(callbacks)
  }

  "One processor is not accessible" in {
    when(callbacks.apply(aFailable)) thenReturn Future { Failure { new InaccessibleException } }
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
    verify(callbacks, times(2)).apply(aFailable)
    verify(callbacks, times(3)).apply(bFailable)
    verifyNoMoreInteractions(callbacks)
  }

  "Both processors are accessible again, but delay as not elapsed" in {
    when(callbacks.apply(aFailable)) thenReturn Future { Success(xResult) }
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
    verify(callbacks, times(2)).apply(aFailable)
    verify(callbacks, times(5)).apply(bFailable)
    verifyNoMoreInteractions(callbacks)
  }

  "Delay has elapsed" in {
    _now += TestDelay
    runSelector().value shouldEqual Some(Success(aFailable → xResult))
    runSelector().value shouldEqual Some(Success(bFailable → yResult))
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
    pendingFuture.value shouldEqual Some(Success(aFailable → xResult))
    verify(callbacks, times(5)).apply(aFailable)
    verify(callbacks, times(7)).apply(bFailable)
    verifyNoMoreInteractions(callbacks)
  }

  "Other failure aborts FailableSelector" in {
    when(callbacks.apply(bFailable)) thenReturn Future { throw BadException }
    runSelector().value shouldEqual Some(Success(aFailable → xResult))
    runSelector().value shouldEqual Some(Failure(BadException))
    verify(callbacks, times(6)).apply(aFailable)
    verify(callbacks, times(8)).apply(bFailable)
    verifyNoMoreInteractions(callbacks)
  }

  "One more failure" in {
    when(callbacks.apply(aFailable)) thenReturn Future { throw BadException }
    runSelector().value shouldEqual Some(Failure(BadException))
    verify(callbacks, times(7)).apply(aFailable)
    verifyNoMoreInteractions(callbacks)
  }

  private def runSelector(): Future[(Failable, Result)] = {
    val selector = new FailableSelector(failables, callbacks, callQueue) {
      override def now = _now
    }
    val future = selector.start()
    callRunner.executeMatureCalls()
    future
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
}
