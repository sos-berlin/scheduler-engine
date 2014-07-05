package com.sos.scheduler.engine.common.scalautil

import ScalaUtils._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._

final class ScalaUtilsTest extends FreeSpec {

  "toImmutableSeq of an already immutable.Seq" in {
    val list = List(1, 2, 3)
    assert(list.toImmutableSeq eq list)
  }

  "countEquals" in {
    Iterator(11, 22, 33, 22, 33, 33).countEquals shouldEqual Map(11 -> 1, 22 -> 2, 33 -> 3)
    List[Int]() shouldEqual Map[Int, Int]()
  }

  "toKeyedMap" in {
    case class A(name: String, i: Int)
    List(A("eins", 1), A("zwei", 2)) toKeyedMap { _.i } shouldEqual Map(1 -> A("eins", 1), 2 -> A("zwei", 2))
  }

  "cast" in {
    val s: Any = "Hej!"
    val string = cast[String](s)
    string shouldEqual "Hej!"
    intercept[ClassCastException]{ cast[String](1) } .getMessage should include ("expected instead of")
  }

  "someUnless" in {
    someUnless(7, none = 0) shouldEqual Some(7)
    someUnless(0, none = 0) shouldEqual None
  }
}
