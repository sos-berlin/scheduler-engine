package com.sos.scheduler.engine.tests.scheduler.scheduler_log

import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class SchedulerLogCommandIT extends ScalaSchedulerTest {
  test("scheduler_log.log_categories.show") {
    val result = scheduler executeXml <scheduler_log.log_categories.show/>
    result.string should include ("idispatch")   // Zum Beispiel
  }
}
