package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.common.soslicense.LicenseKeyBunch
import com.sos.scheduler.engine.common.soslicense.Parameters.UniversalAgent
import java.util.concurrent.atomic.{AtomicBoolean, AtomicInteger}
import org.jetbrains.annotations.TestOnly

/**
  * @author Joacim Zschimmer
  */
trait LicensedTaskLimiter {

  def errorMessage(taskCount: Int): String

  private val _count = new AtomicInteger

  def countTask[A](licenseKeyBunch: LicenseKeyBunch)(body: ⇒ A): (RemoveTask, A) =
    try {
      val n = _count.getAndIncrement()
      if (n >= 1) licenseKeyBunch.require(UniversalAgent, errorMessage(n))
      (new RemoveTask, body)
    }
    catch { case t: Throwable ⇒
      _count.decrementAndGet()
      throw t
    }

  @TestOnly
  private[task] def count = _count.get

  final class RemoveTask private[LicensedTaskLimiter] {
    private val called = new AtomicBoolean

    def apply(): Unit = {
      if (!called.getAndSet(true)) {
        _count.decrementAndGet()
      }
    }
  }
}
