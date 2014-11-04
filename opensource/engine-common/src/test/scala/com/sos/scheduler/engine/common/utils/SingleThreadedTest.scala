package com.sos.scheduler.engine.common.utils

import com.sos.scheduler.engine.common.utils.SingleThreadedTest._
import org.scalatest.FreeSpec
import org.scalatest.concurrent.AsyncAssertions.Waiter

/**
 * @author Joacim Zschimmer
 */
final class SingleThreadedTest extends FreeSpec {

  private val a = new A

  "In same thread: okay" in {
    a.f()
  }

  "In other thread: reject" in {
    val w = new Waiter
    new Thread {
      override def run(): Unit = {
        w {
          intercept[IllegalStateException] {
            a.f()
          }
        }
        w.dismiss()
      }
    }
  }
}

private object SingleThreadedTest {
  class A extends SingleThreaded {
    def f() = requireMyThread()
  }
}
