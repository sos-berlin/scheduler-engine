package com.sos.scheduler.engine.common.async

import com.sos.scheduler.engine.common.time.ScalaJoda.{DurationRichInt, RichInstant, sleep}
import org.joda.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.{OneInstancePerTest, FunSuite}

@RunWith(classOf[JUnitRunner])
class CallRunnerTest extends FunSuite with OneInstancePerTest {
  private val callDispatcher = new CallRunner(new StandardCallQueue)

  test("(Warm up)") {  // Für die langsamen SOS-Rechner
    callDispatcher.queue add TimedCall(now()) {}
    callDispatcher.execute()
    1 should equal (1)
  }

  test("CallRunner runs TimedCall at scheduled instants") {
    val delay = 500.ms
    var a = 0
    callDispatcher.queue add { () => a += 1 }
    callDispatcher.queue add TimedCall(now() + delay) { a += 10 }
    callDispatcher.queue add { () => a += 100 }
    callDispatcher.execute()
    a should equal (101)
    sleep(delay)
    callDispatcher.execute()
    a should equal (111)
  }
}
