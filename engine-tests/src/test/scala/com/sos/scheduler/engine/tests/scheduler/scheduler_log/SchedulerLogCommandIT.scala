package com.sos.scheduler.engine.tests.scheduler.scheduler_log

import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
private class SchedulerLogCommandIT extends FunSuite with ScalaSchedulerTest {
  test("scheduler_log.log_categories.show") {
    val result = scheduler executeXml <scheduler_log.log_categories.show/>
    result.string should include ("idispatch")   // Zum Beispiel
  }
}
