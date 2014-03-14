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
    Nil.countEquals shouldEqual Map()
  }

  "toKeyedMap" in {
    case class A(name: String, i: Int)
    List(A("eins", 1), A("zwei", 2)) toKeyedMap { _.i } shouldEqual Map(1 -> A("eins", 1), 2 -> A("zwei", 2))
  }
}
