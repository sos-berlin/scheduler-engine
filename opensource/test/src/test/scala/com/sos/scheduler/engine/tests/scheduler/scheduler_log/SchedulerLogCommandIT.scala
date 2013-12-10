package com.sos.scheduler.engine.tests.scheduler.scheduler_log

import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class SchedulerLogCommandIT extends ScalaSchedulerTest {
  test("scheduler_log.log_categories.show") {
    val result = scheduler executeXml <scheduler_log.log_categories.show/>
    result.string should include ("idispatch")   // Zum Beispiel
  }
}
