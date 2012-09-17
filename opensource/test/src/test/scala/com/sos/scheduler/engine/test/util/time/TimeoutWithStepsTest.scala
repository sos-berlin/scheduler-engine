package com.sos.scheduler.engine
package test.util.time

import com.sos.scheduler.engine.test.util.time.TimeoutWithSteps._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class TimeoutWithStepsTest extends FunSuite {
  test("instantIterator") {
    millisInstantIterator(100, 7, 7).toList should equal(Seq(100, 100 + 7))
    millisInstantIterator(100, 7, 3).toList should equal(Seq(100, 100 + 3, 100 + 6, 100 + 7))
    millisInstantIterator(100, 3, 7).toList should equal(Seq(100, 100 + 3))
  }
}
