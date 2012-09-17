package com.sos.scheduler.engine
package test.util.time

import com.sos.scheduler.engine.test.util.time.WaitForCondition._
import java.lang.System.currentTimeMillis
import org.joda.time.DateTime.now
import org.joda.time.Duration.millis
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class WaitForConditionTest extends FunSuite {

  import WaitForConditionTest._

  test("realTimeIterator (time critical test)") {
    realTimeIterator(Seq(now().getMillis)) // Aufruf zum Warmwerden. Laden der Klasse kann eine Weile dauern
    meterElapsedTime {realTimeIterator(Seq(now().getMillis + 10000))} should be < (50L) // Bereitstellung soll nicht warten
    val t0 = now().getMillis
    val (t1, t2, t3) = (t0 + 100, t0 + 300, t0 + 400)
    val i = realTimeIterator(Seq(t1, t2, t3))
    meterElapsedTime {i.next()} should be(t1 - t0 plusOrMinus 50)
    meterElapsedTime {i.next()} should be(t2 - t1 plusOrMinus 50)
    meterElapsedTime {i.next()} should be(t3 - t2 plusOrMinus 50)
  }

  test("waitForCondition(TimeoutWithSteps) 0 steps (time critical test)") {
    val elapsed = meterElapsedTime {
      waitForCondition(TimeoutWithSteps(millis(2000), millis(1000)))(true)
    }
    elapsed.toInt should be < (50)
  }

  test("waitForCondition(TimeoutWithSteps) all steps (time critical test)") {
    val elapsed = meterElapsedTime {
      waitForCondition(TimeoutWithSteps(millis(100), millis(1)))(false)
    }
    elapsed.toInt should (be >= 100 and be <= 200)
  }

  test("waitForCondition(TimeoutWithSteps) some steps (time critical test)") {
    var cnt = 0
    val elapsed = meterElapsedTime {waitForCondition(TimeoutWithSteps(millis(100), millis(10))) {cnt += 1; cnt > 3}}
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
