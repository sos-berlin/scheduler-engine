package com.sos.scheduler.engine.common.async

import com.sos.scheduler.engine.common.async.FutureCompletion.{futureCall, futureTimedCall, callFuture}
import com.sos.scheduler.engine.common.time.ScalaJoda._
import org.joda.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{OneInstancePerTest, FunSuite}
import scala.util.Success

@RunWith(classOf[JUnitRunner])
final class FutureCompletionTest extends FunSuite with OneInstancePerTest {

  private lazy val queue: PoppableCallQueue = new StandardCallQueue
  private lazy val dispatcher = new CallRunner(queue)

  test("warm-up") {
    val call = futureTimedCall(now()) {}
    queue.add(call)
    val future = call.future
    dispatcher.executeMatureCalls()
    future.isCompleted
  }

  test("Success") {
    val call = futureTimedCall(now() + 200.ms) { "Hej!" }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.executeMatureCalls()
    (future.isCompleted, future.value) should be (false, None)
    sleep(220.ms)
    dispatcher.executeMatureCalls()
    (future.isCompleted, future.value) should be (true, Some(Success("Hej!")))
  }

  test("Failure") {
    val call = futureTimedCall(now() + 100.ms) { throw new TestException }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.executeMatureCalls()
    (future.isCompleted, future.value) should be (false, None)
    sleep(101.ms)
    dispatcher.executeMatureCalls()
    future.isCompleted should be (true)
    intercept[TestException] { future.value.get.get }
  }

  test("ShortTermCall Success") {
    val call = futureCall { "Hej!" }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.executeMatureCalls()
    (future.isCompleted, future.value) should be (true, Some(Success("Hej!")))
  }

  test("ShortTermCall Failure") {
    val call = futureCall { throw new TestException }
    queue.add(call)
    val future = call.future
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.executeMatureCalls()
    future.isCompleted should be (true)
    intercept[TestException] { future.value.get.get }
  }

  test("callFuture") {
    implicit val implicitQueue = queue
    val future = callFuture { throw new TestException }
    (future.isCompleted, future.value) should be (false, None)
    dispatcher.executeMatureCalls()
    future.isCompleted should be (true)
    intercept[TestException] { future.value.get.get }
  }

  private case class TestException() extends RuntimeException
}

