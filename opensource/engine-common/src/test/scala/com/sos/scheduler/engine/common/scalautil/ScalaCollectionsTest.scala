package com.sos.scheduler.engine.common.scalautil

import ScalaCollections.{RichTraversable, emptyToNone}
import ScalaCollectionsTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class ScalaCollectionsTest extends FreeSpec {

  "duplicateKeys" in {
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

  "requireDistinct" in {
    def r(o: Seq[A]) = o requireDistinct { _.i }

    r(Seq[A]()) should be ('empty)
    intercept[Exception] { r(Seq(a1, a2)) }
  }

  "emptyToNone" in {
    emptyToNone("") shouldEqual None
    emptyToNone("x") shouldEqual Some("x")
    emptyToNone(Nil) shouldEqual None
    emptyToNone(List(1)) shouldEqual Some(List(1))
    emptyToNone(Array[Int]()) shouldEqual None
    val a = Array(1)
    emptyToNone(a) shouldEqual Some(a)
  }
}

private object ScalaCollectionsTest {
  private case class A(i: Int, s: String)

  private val a1 = A(1, "eins")
  private val a2 = A(1, "ett")
  private val b1 = A(2, "zwei")
  private val c1 = A(3, "drei")
  private val c2 = A(3, "tre")
  private val c3 = A(3, "tri")
}