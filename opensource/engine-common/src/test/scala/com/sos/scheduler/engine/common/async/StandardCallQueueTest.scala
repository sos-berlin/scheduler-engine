package com.sos.scheduler.engine.common.async

import java.lang.System.currentTimeMillis
import java.lang.Thread.sleep
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.{OneInstancePerTest, FunSuite}

@RunWith(classOf[JUnitRunner])
class StandardCallQueueTest extends FunSuite with OneInstancePerTest {
  private val callQueue = new StandardCallQueue

  test("add(=>A)") {
    var a = 0
    callQueue add { () => a += 1 }
    val call = callQueue.popMature().get
    call.at should equal (0)
    call()
    a should equal (1)
    callQueue.popMature() should equal (None)
  }

  test("add(short_term, TimedCall)") {
    var a = 0
    val call = TimedCall(TimedCall.shortTerm) { a += 1 }
    callQueue add call
    val c = callQueue.popMature().get
    c should be theSameInstanceAs (call)
    c()
    a should equal (1)
    callQueue.popMature() should equal (None)
  }

  test("add(at, TimedCall)") {
    var a = 0
    val call = TimedCall(currentTimeMillis() + 50) { a += 1 }
    callQueue.add(call)
    callQueue.popMature() should equal (None)
    sleep(10)
    callQueue.popMature() should equal (None)
    sleep(42)
    val c = callQueue.popMature().get
    c should be theSameInstanceAs (call)
    c()
    a should equal (1)
    callQueue.popMature() should equal (None)
  }

  test("remove") {
    val call = TimedCall(currentTimeMillis() + 50) {}
    callQueue.add(call)
    callQueue.remove(call)
    callQueue.tryRemove(call) should equal (false)
    callQueue should be ('empty)
    intercept[RuntimeException] { callQueue.remove(call) }
  }
}
