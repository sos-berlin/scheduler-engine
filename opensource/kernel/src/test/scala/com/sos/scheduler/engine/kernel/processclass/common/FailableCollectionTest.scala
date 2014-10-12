package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.kernel.processclass.common.FailableCollectionTest._
import org.joda.time.{Duration, Instant}
import org.scalatest.FreeSpec

/**
 * @author Joacim Zschimmer
 */
final class FailableCollectionTest extends FreeSpec {

  "Requirements" - {
    "Minimum one entity" in {
      intercept[IllegalArgumentException] {
        new FailableCollection(List[A](), 100.ms)
      }
    }

    "Entities must be distinct" in {
      intercept[IllegalArgumentException] {
        new FailableCollection(List(A(1), A(1)), 100.ms)
      }
    }
  }

  "Behaviour sequence" - {
    val startInstant = new Instant(10 * 24 * 3600 * 1000) // Some time
    var _now = startInstant
    val as = 0 to 2 map A
    val delay = 30.s
    val failureCollection = new FailableCollection[A](as, failureTimeout = delay) {
      override def now = _now
    }

    "No failures" in {
      check(
        0.s → A(0),
        0.s → A(1),
        0.s → A(2),
        0.s → A(0),
        0.s → A(1))
    }

    "First fails" in {
      failureCollection.setFailure(A(1), new Exception)
      check(
        0.s → A(2),
        0.s → A(0),
        0.s → A(2),
        0.s → A(0))
    }

    "Second fails" in {
      _now = startInstant + 10.s
      failureCollection.setFailure(A(2), new Exception)
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "All have failed" in {
      failureCollection.setFailure(A(0), new Exception)
      check(
        20.s → A(1),
        20.s → A(1),
        20.s → A(1),
        20.s → A(1))
    }

    "First failure timed out" in {
      _now = startInstant + delay
      check(
        0.s → A(1),
        0.s → A(1),
        0.s → A(1),
        0.s → A(1))
    }

    "Extra clearFailure on third failure" in {
      _now = startInstant + delay
      failureCollection.clearFailure(A(2))
      check(
        0.s → A(2),
        0.s → A(1),
        0.s → A(2),
        0.s → A(1))
    }

    "All failures timed out" in {
      _now = startInstant + delay + 10.s
      check(
        0.s → A(2),
        0.s → A(0),
        0.s → A(1),
        0.s → A(2))
    }

    "clearFailure for timed-out failure does not make a difference" in {
      failureCollection.clearFailure(A(1))
      check(
        0.s → A(0),
        0.s → A(1),
        0.s → A(2),
        0.s → A(0))
    }

    def check(delayAndAs: (Duration, A)*): Unit = {
      assertResult(delayAndAs) {
        List.tabulate(delayAndAs.size) { _ ⇒ failureCollection.nextDelayAndEntity() }
      }
    }
  }
}

private object FailableCollectionTest {
  final case class A(id: Int)
}
