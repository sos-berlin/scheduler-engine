package com.sos.scheduler.engine.plugins.jetty

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.kernel.util.Time

@RunWith(classOf[JUnitRunner])
final class DebugTest extends ScalaSchedulerTest with CheckedBeforeAll {
  ignore("(for debugging only") {
    controller.waitForTermination(Time.of(3600))
  }
}