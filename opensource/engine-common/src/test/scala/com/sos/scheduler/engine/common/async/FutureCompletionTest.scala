package com.sos.scheduler.engine.common.async

import com.sos.scheduler.engine.common.async.FutureCompletion.futureCall
import java.lang.System.currentTimeMillis
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.{OneInstancePerTest, FunSuite}
import scala.util.Success

@RunWith(classOf[JUnitRunner])
class FutureCompletionTest extends FunSuite with OneInstancePerTest {

  private lazy val queue = new CallQueue
  private lazy val dispatcher = new CallDispatcher(queue)

  test("Success") {
    val call = futureCall(currentTimeMillis() + 100) { "Hej!" }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.execute()
    (future.isCompleted, future.value) should be (false, None)
    Thread.sleep(100+1)
    dispatcher.execute()
    (future.isCompleted, future.value) should be (true, Some(Success("Hej!")))
  }

  test("Failure") {
    val call = futureCall(currentTimeMillis() + 100) { throw new TestException }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.execute()
    (future.isCompleted, future.value) should be (false, None)
    Thread.sleep(100+1)
    dispatcher.execute()
    future.isCompleted should be (true)
    intercept[TestException] { future.value.get.get }
  }

  test("ShortTermCall Success") {
    val call = futureCall() { "Hej!" }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.execute()
    (future.isCompleted, future.value) should be (true, Some(Success("Hej!")))
  }

  test("ShortTermCall Failure") {
    val call = futureCall() { throw new TestException }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.execute()
    future.isCompleted should be (true)
    intercept[TestException] { future.value.get.get }
  }

  private case class TestException() extends RuntimeException
}

