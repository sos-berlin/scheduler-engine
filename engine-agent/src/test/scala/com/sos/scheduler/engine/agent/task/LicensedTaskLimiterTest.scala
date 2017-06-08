package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.common.soslicense.{LicenseKeyBunch, LicenseKeyParameterIsMissingException}
import java.util.NoSuchElementException
import org.scalatest.FreeSpec

/**
  * @author Joacim Zschimmer
  */
final class LicensedTaskLimiterTest extends FreeSpec {

  private val keyBunch = LicenseKeyBunch("SOS-DEMO-1-D3Q-1AWS-ZZ-ITOT9Q6")
  private val limiter = new LicensedTaskLimiter {
    def errorMessage(n: Int) = "ERROR"
  }

  "With Agent license" in {
    for (_ ← 1 to 2) {
      val (removeTask1, x) = limiter.countTask(keyBunch) { 7 }
      assert(x == 7)
      assert(limiter.count == 1)
      val (removeTask2, _) = limiter.countTask(keyBunch) {}
      assert(limiter.count == 2)
      removeTask1()
      assert(limiter.count == 1)
      removeTask2()
      assert(limiter.count == 0)
      removeTask1()
      assert(limiter.count == 0)
      removeTask2()
      assert(limiter.count == 0)
    }
  }

  "With error" in {
    for (_ ← 1 to 2) {
      intercept[NoSuchElementException] {
        limiter.countTask(keyBunch) { throw new NoSuchElementException }
      }
      assert(limiter.count == 0)
    }
  }

  "Without Agent license" in {
    for (_ ← 1 to 2) {
      val (removeTask, ()) = limiter.countTask(LicenseKeyBunch()) {}
      assert(limiter.count == 1)
      intercept[LicenseKeyParameterIsMissingException] {
        limiter.countTask(LicenseKeyBunch()) {}
      }
      assert(limiter.count == 1)
      removeTask()
      assert(limiter.count == 0)
    }
  }
}
