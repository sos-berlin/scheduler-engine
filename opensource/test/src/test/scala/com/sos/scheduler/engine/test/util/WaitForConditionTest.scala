package com.sos.scheduler.engine.test.util

import com.sos.scheduler.engine.test.util.WaitForCondition._
import java.lang.System.currentTimeMillis
import org.joda.time.DateTime.now
import org.joda.time.Duration.millis
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._

final class WaitForConditionTest extends FunSuite {

  import WaitForConditionTest._

  test("instantIterator") {
    millisInstantIterator(100, 7, 7).toList should equal (Seq(100, 100+7))
    millisInstantIterator(100, 7, 3).toList should equal (Seq(100, 100+3, 100+6, 100+7))
    millisInstantIterator(100, 3, 7).toList should equal (Seq(100, 100+3))
  }

  test("realTimeIterator (time critical test)") {
    realTimeIterator(Seq(now().getMillis))    // Aufruf zum Warmwerden. Laden der Klasse kann eine Weile dauern
    meterElapsedTime { realTimeIterator(Seq(now().getMillis + 10000)) } should be < (50L)   // Bereitstellung soll nicht warten
    val t0 = now().getMillis
    val (t1, t2, t3) = (t0 + 100, t0 + 300, t0 + 400)
    val i = realTimeIterator(Seq(t1, t2, t3))
    meterElapsedTime { i.next() } should be (t1 - t0 plusOrMinus 50)
    meterElapsedTime { i.next() } should be (t2 - t1 plusOrMinus 50)
    meterElapsedTime { i.next() } should be (t3 - t2 plusOrMinus 50)
  }

  test("waitForCondition(TimeoutWithSteps) 0 steps (time critical test)") {
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(millis(2000), millis(1000))) { true } }
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
    (currentTimeMillis() - start)
  }
}
