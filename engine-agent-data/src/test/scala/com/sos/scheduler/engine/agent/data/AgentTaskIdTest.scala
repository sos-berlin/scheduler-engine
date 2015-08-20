package com.sos.scheduler.engine.agent.data

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentTaskIdTest extends FreeSpec {

  "value" in {
    assert(AgentTaskId(123, 789).value == 123000000789L)
    assert(AgentTaskId(123, -789).value == 123000000789L)
    assert(AgentTaskId(123, 999999999).value == 123999999999L)
    assert(AgentTaskId(-123, 789).value == -123000000789L)
    assert(AgentTaskId(-123, -789).value == -123000000789L)
    assert(AgentTaskId(-123, 999999999).value == -123999999999L)
  }

  "toString" in {
    assert(AgentTaskId(123, 789).toString == "AgentTaskId(123-789)")
    assert(AgentTaskId(-123, 789).toString == "AgentTaskId(-123-789)")
  }

  "apply(number)" in {
    assert(AgentTaskId(123000000789L) == AgentTaskId(123, 789))
    assert(AgentTaskId(-123000000789L) == AgentTaskId(-123, 789))
  }

  "apply(string)" in {
    assert(AgentTaskId("123-789").string == "123-789")
    assert(AgentTaskId("-123-789").string == "-123-789")
    assert(AgentTaskId("123000000789").string == "123-789")
    assert(AgentTaskId("-123000000789").string == "-123-789")
    intercept[IllegalArgumentException] { AgentTaskId("") }
    intercept[IllegalArgumentException] { AgentTaskId("123-") }
    intercept[IllegalArgumentException] { AgentTaskId("-789-") }
  }

  "AgentTaskIdGenerator" in {
    val next = AgentTaskId.newGenerator().next _
    for (i ← 1 to 10000) {
      val id = next()
      assert(id.value >= 1)
      assert(id.index == i)
    }
  }

  "AgentTaskIdGenerator overflow" in {
    val start = Int.MaxValue - 100
    val iterator = AgentTaskId.newGenerator(start = start)
    for (_ ← 0 to 10000) { assert(iterator.next().value >= 1) }
  }

  "AgentTaskIdGenerator overflow 2" in {
    val ids = (AgentTaskId.newGenerator(start = Int.MaxValue - 2) take 5).toList
    ids map { _.index } shouldEqual List(Int.MaxValue - 2, Int.MaxValue - 1, Int.MaxValue, 1, 2)
  }
}
