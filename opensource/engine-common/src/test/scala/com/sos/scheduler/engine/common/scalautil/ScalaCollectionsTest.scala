package com.sos.scheduler.engine.common.scalautil

import ScalaCollections.RichTraversable
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import ScalaCollectionsTest._

@RunWith(classOf[JUnitRunner])
class ScalaCollectionsTest extends FunSuite {

  test("duplicateKeys") {
    def dup(o: Seq[A]) = o duplicates { _.i }

    dup(Seq[A]()) should be ('empty)
    dup(Seq(a1)) should be ('empty)
    dup(Seq(a1, b1)) should be ('empty)
    dup(Seq(a1, b1, c1)) should be ('empty)
    dup(Seq(a1, a1)) should equal (Map(1 -> Seq(a1, a1)))
    dup(Seq(a1, a2)) should equal (Map(1 -> Seq(a1, a2)))
    dup(Seq(a1, a2, b1)) should equal (Map(1 -> Seq(a1, a2)))
    dup(Seq(a1, a2, b1, c1, c2, c3)) should equal (Map(1 -> Seq(a1, a2), 3 -> Seq(c1, c2, c3)))
  }

  test("requireDistinct") {
    def r(o: Seq[A]) = o requireDistinct { _.i }

    r(Seq[A]()) should be ('empty)
    intercept[Exception] { r(Seq(a1, a2)) }
  }
}

private object ScalaCollectionsTest {
  case class A(i: Int, s: String)

  val a1 = A(1, "eins")
  val a2 = A(1, "ett")
  val b1 = A(2, "zwei")
  val c1 = A(3, "drei")
  val c2 = A(3, "tre")
  val c3 = A(3, "tri")
}