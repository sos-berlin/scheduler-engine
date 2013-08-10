package com.sos.scheduler.engine.test.util.time

import WaitForConditionTest._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.test.util.time.WaitForCondition._
import java.lang.System.currentTimeMillis
import org.joda.time.DateTime.now
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.joda.time.Duration

@RunWith(classOf[JUnitRunner])
final class WaitForConditionTest extends FunSuite {

  test("realTimeIterator (time-critical test)") {
    realTimeIterator(Seq(now().getMillis)) // Aufruf zum Warmwerden. Laden der Klasse kann eine Weile dauern
    meterElapsedTime { realTimeIterator(Seq((now() + 10.s).getMillis)) } should be < 300.ms // Bereitstellung soll nicht warten
    val t0 = now().getMillis
    val (t1, t2, t3) = (t0 + 500, t0 + 1500, t0 + 2000)
    val i = realTimeIterator(Seq(t1, t2, t3))
    meterElapsedTime { i.next() } .getMillis should be (t1 - t0 plusOrMinus 400)
    meterElapsedTime { i.next() } .getMillis should be (t2 - t1 plusOrMinus 400)
    meterElapsedTime { i.next() } .getMillis should be (t3 - t2 plusOrMinus 400)
  }

  test("waitForCondition(TimeoutWithSteps) 0 steps (time-critical test)") {
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(2.s, 1.s)) { true } }
    elapsed should be < 500.ms
  }

  test("waitForCondition(TimeoutWithSteps) all steps (time-critical test)") {
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(300.ms, 1.ms)) { false } }
    elapsed should (be >= 300.ms and be <= 700.ms)
  }

  test("waitForCondition(TimeoutWithSteps) some steps (time-critical test)") {
    var cnt = 0
    val elapsed = meterElapsedTime { waitForCondition(TimeoutWithSteps(1.s, 10.ms)) { cnt += 1; cnt > 10 } }  // 100ms
    elapsed should (be >= 100.ms and be < 400.ms)
  }
}

private object WaitForConditionTest {
  def meterElapsedTime(f: => Unit): Duration = {
    val start = currentTimeMillis()
    f
    (currentTimeMillis() - start).ms
  }
}
