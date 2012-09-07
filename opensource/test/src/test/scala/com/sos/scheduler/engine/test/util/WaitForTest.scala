package com.sos.scheduler.engine.test.util

import com.sos.scheduler.engine.test.util.WaitFor._
import org.joda.time.DateTime.now
import org.joda.time.Duration.{millis, standardHours}
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._

final class WaitForTest extends FunSuite {

  import WaitForTest._

  test("durationSeq") {
    durationSeq(millis(7), millis(3)).toSeq should equal (Seq(millis(3), millis(6), millis(7)))
    durationSeq(millis(3), millis(7)).toSeq should equal (Seq(millis(3)))
  }

  test("waitTimeNowIterator") {
    val i = waitTimeNowIterator(Seq(now() plus standardHours(7)))
    i.next().getMillis should be (7*60*60*1000L plusOrMinus 1000L)
    i.hasNext should be (false)
  }

  test("pointIterator") {
    val t = now()
    val i = pointIterator(t, Seq(standardHours(7)))
    (i.next().getMillis - t.getMillis) should be (7*60*60*1000L plusOrMinus 1000L)
    i.hasNext should be (false)
  }

  test("waitTimeNowIterator - empty") {
    val i = waitTimeNowIterator(Seq(now()))
    i.hasNext should be (false)
  }

  test("waitFor(duration, step) 0 steps") {
    val elapsed = meterElapsedTime { waitFor(millis(1000), millis(1)) { true } }
    elapsed.toInt should be < (50)
  }

  test("waitFor(duration, step) all steps") {
    val elapsed = meterElapsedTime { waitFor(millis(100), millis(1)) { false } }
    elapsed.toInt should (be >= 100 and be <= 200)
  }

  test("waitFor(duration, step) some steps") {
    var cnt = 0
    val elapsed = meterElapsedTime { waitFor(millis(100), millis(10)) { cnt += 1; cnt > 3 } }
    elapsed.toInt should (be >= 30 and be < 90)
  }
}

private object WaitForTest {
  def meterElapsedTime(f: => Unit) = {
    val start = System.currentTimeMillis()
    f
    System.currentTimeMillis() - start
  }
}

