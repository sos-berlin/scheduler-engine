package com.sos.scheduler.engine.common.async

import FutureTest._
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{OneInstancePerTest, FunSuite}
import org.slf4j.LoggerFactory
import scala.collection.mutable
import scala.concurrent._
import scala.concurrent.future
import scala.sys.error
import scala.util.Success

@RunWith(classOf[JUnitRunner])
class FutureTest extends FunSuite with OneInstancePerTest {
  implicit private val executionContext = new QueuedExecutionContextExecutor

  test("Success") {
    val f = future { "Hallo!" }
    f.isCompleted should be (false)
    executionContext.run()
    f.isCompleted should be (true)
    f.value should equal (Some(Success("Hallo!")))
  }

  test("Failure") {
    val f = future { throw TestException() }
    f.isCompleted should be (false)
    executionContext.run()
    f.isCompleted should be (true)
    intercept[TestException] { f.value.get.get }
  }
}

private object FutureTest {
  val logger = LoggerFactory.getLogger(classOf[FutureTest])

  private case class TestException() extends RuntimeException

  class QueuedExecutionContextExecutor extends ExecutionContextExecutor {
    private val queue = mutable.UnrolledBuffer[Runnable]()

    def execute(runnable: Runnable) {
      queue += runnable
    }

    def run() {
      queue foreach { r => r.run() }
      queue.remove(0, queue.size)
    }

    def reportFailure(t: Throwable) {
      //logger.error(s"reportFailure $t", t)
      error(t.toString)
    }
  }
}
