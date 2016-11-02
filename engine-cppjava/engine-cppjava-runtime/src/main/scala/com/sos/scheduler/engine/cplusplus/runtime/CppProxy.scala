package com.sos.scheduler.engine.cplusplus.runtime

import com.sos.scheduler.engine.cplusplus.runtime.annotation.JavaOnlyInterface
import java.lang.Thread.currentThread
import org.slf4j.LoggerFactory

@JavaOnlyInterface
trait CppProxy {
  def cppReferenceIsValid: Boolean
}

object CppProxy {
  private val logger = LoggerFactory.getLogger(getClass.getName stripSuffix "$")
  private var requiredCppThread: Thread = null

  def requireCppThread() {
    if (currentThread ne requiredCppThread) {
      val t = new IllegalStateException(s"Not in C++ thread. This is '$currentThread', expected is '$requiredCppThread'")
      logger.error(t.toString, t)
      throw t
    }
  }

  def cppThreadStarted() {
    require(requiredCppThread == null || requiredCppThread == currentThread)
    requiredCppThread = currentThread
  }

  def cppThreadEnded(): Unit = {
    requiredCppThread = null
  }
}
