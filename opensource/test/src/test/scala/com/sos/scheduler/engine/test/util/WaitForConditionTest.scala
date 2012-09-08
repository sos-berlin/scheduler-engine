package com.sos.scheduler.engine.test.util

import com.sos.scheduler.engine.test.util.WaitForCondition._
import org.joda.time.DateTime.now
import org.joda.time.Duration.millis
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._
import java.lang.System.currentTimeMillis

final class WaitForConditionTest extends FunSuite {

  import WaitForConditionTest._

  test("millisFromNowUntilInstanceIterator") {
    val i = millisFromNowUntilInstanceIterator(Seq(now().getMillis + 70000))
    i.next() should be (70000L plusOrMinus 1000L)
    i.hasNext should be (false)
  }

  test("millisFromNowUntilInstanceIterator - empty") {
    val i = millisFromNowUntilInstanceIterator(Seq(now().getMillis))
    i.hasNext should be (false)
  }

  test("instantIterator") {
    millisInstantIterator(100, 7, 7).toList should equal (Seq(100+7))
    millisInstantIterator(100, 7, 3).toList should equal (Seq(100+3, 100+6, 100+7))
    millisInstantIterator(100, 3, 7).toList should equal (Seq(100+3))
  }

  test("waitForCondition(TimeoutWithSteps) 0 steps (time critical test)") {
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(millis(1000), millis(1))) { true } }
    elapsed.toInt should be < (50)
  }

  test("waitForCondition(TimeoutWithSteps) all steps (time critical test)") {
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(millis(100), millis(1))) { false } }
    elapsed.toInt should (be >= 100 and be <= 200)
  }

  test("waitForCondition(TimeoutWithSteps) some steps (time critical test)") {
    var cnt = 0
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(millis(100), millis(10))) { cnt += 1; cnt > 3 } }
    elapsed.toInt should (be >= 30 and be < 90)
  }
}

private object WaitForConditionTest {
  def meterElapsedTime(f: => Unit) = {
    val start = currentTimeMillis()
    f
    currentTimeMillis() - start
  }
}
