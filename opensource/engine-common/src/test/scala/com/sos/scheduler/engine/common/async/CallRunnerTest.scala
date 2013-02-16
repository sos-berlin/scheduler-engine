package com.sos.scheduler.engine.common.async

import java.lang.System.currentTimeMillis
import java.lang.Thread.sleep
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.{OneInstancePerTest, FunSuite}

@RunWith(classOf[JUnitRunner])
class CallRunnerTest extends FunSuite with OneInstancePerTest {
  private val callDispatcher = new CallRunner(new StandardCallQueue)

  test("X") {
    var a = 0
    callDispatcher.queue add { () => a += 1 }
    callDispatcher.queue add TimedCall(currentTimeMillis() + 100) { a += 10 }
    callDispatcher.queue add { () => a += 100 }
    callDispatcher.execute()
    a should equal (101)
    sleep(101)
    callDispatcher.execute()
    a should equal (111)
  }
}
