package com.sos.scheduler.engine.common.scalautil

import com.sos.scheduler.engine.common.scalautil.ModifiedBy.modifiedBy
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class ModifiedByTest extends FunSuite {
  test("modifiedBy") {
    val a = A(1) modifiedBy { _.x = 2 }
    a should equal (A(2))
  }

  private case class A(var x: Int)
}
