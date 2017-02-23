package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.processclass.common.FailableCollectionTest._
import com.sos.scheduler.engine.kernel.processclass.common.selection.{RoundRobin, FixedPriority}
import java.time.{Duration, Instant}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FailableCollectionTest extends FreeSpec {

  private val as = 0 to 2 map A
  private val delay = 30.s
  private val startInstant = Instant.ofEpochSecond(10 * 24 * 3600) // Some time

  "Requirements" - {
    "Minimum one entity" in {
      intercept[IllegalArgumentException] {
        new FailableCollection(List[A](), () ⇒ 100.ms, FixedPriority)
      }
    }

    "Entities must be distinct" in {
      intercept[IllegalArgumentException] {
        new FailableCollection(List(A(1), A(1)), () ⇒ 100.ms, FixedPriority)
      }
    }
  }

  "Fixed priority" - {
    var _now = startInstant
    implicit val failureCollection = new FailableCollection(as, failureTimeout = () ⇒ delay, FixedPriority) {
      override def now = _now
    }

    "No failures" in {
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "First fails" in {
      failureCollection.setFailure(A(0), new Exception)
      check(
        0.s → A(1),
        0.s → A(1),
        0.s → A(1))
    }

    "Second fails" in {
      _now = startInstant + 10.s
      failureCollection.setFailure(A(1), new Exception)
      check(
        0.s → A(2),
        0.s → A(2),
        0.s → A(2),
        0.s → A(2))
    }

    "All have failed" in {
      failureCollection.setFailure(A(2), new Exception)
      check(
        20.s → A(0),
        20.s → A(0),
        20.s → A(0))
    }

    "First failure timed out" in {
      _now = startInstant + delay
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "Extra clearFailure on first failure" in {
      _now = startInstant + delay
      failureCollection.clearFailure(A(0))
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "All failures timed out" in {
      _now = startInstant + delay + 10.s
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "clearFailure for timed-out failure does not make a difference" in {
      failureCollection.clearFailure(A(1))
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "First fails again" in {
      failureCollection.setFailure(A(0), new Exception)
      check(
        0.s → A(1),
        0.s → A(1),
        0.s → A(1))
    }
  }

  "Round robin" - {
    var _now = startInstant
    implicit val failableCollection = new FailableCollection(as, failureTimeout = () ⇒ delay, RoundRobin) {
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

    "Second fails" in {
      failableCollection.setFailure(A(1), new Exception)
      check(
        0.s → A(2),
        0.s → A(0),
        0.s → A(2),
        0.s → A(0))
    }

    "Third fails" in {
      _now = startInstant + 10.s
      failableCollection.setFailure(A(2), new Exception)
      check(
        0.s → A(0),
        0.s → A(0),
        0.s → A(0),
        0.s → A(0))
    }

    "All have failed" in {
      failableCollection.setFailure(A(0), new Exception)
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
      failableCollection.clearFailure(A(2))
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
      failableCollection.clearFailure(A(1))
      check(
        0.s → A(0),
        0.s → A(1),
        0.s → A(2),
        0.s → A(0))
    }
  }

  private def check(delayAndAs: (Duration, A)*)(implicit failableCollection: FailableCollection[A]): Unit = {
    assertResult(delayAndAs) {
      List.tabulate(delayAndAs.size) { _ ⇒ failableCollection.nextDelayAndEntity() }
    }
  }
}

private object FailableCollectionTest {
  final case class A(id: Int)
}
