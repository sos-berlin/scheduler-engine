package com.sos.scheduler.engine.plugins.jetty

import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class UtilsTest extends FunSuite {
  test("randomInt") {
    val range = 1 to 10
    for (i <- 1 to 1000)
      range should contain (Utils.randomInt(range))
  }

  test("randomInts") {
    val range = 1 to 10
    for (i <- 1 to 1000) {
      val o = Utils.randomInts(range).toSeq
      o should have size(range.size)
      o.toSet should have size(range.size)
    }
  }
}
