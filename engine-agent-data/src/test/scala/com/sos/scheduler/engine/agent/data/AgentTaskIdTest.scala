package com.sos.scheduler.engine.agent.data

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
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
}
