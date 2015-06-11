package com.sos.scheduler.engine.agent.data

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentProcessIdTest extends FreeSpec {

  "value" in {
    assert(AgentProcessId(123, 789).value == 123000000789L)
    assert(AgentProcessId(123, -789).value == 123000000789L)
    assert(AgentProcessId(123, 999999999).value == 123999999999L)
    assert(AgentProcessId(-123, 789).value == -123000000789L)
    assert(AgentProcessId(-123, -789).value == -123000000789L)
    assert(AgentProcessId(-123, 999999999).value == -123999999999L)
  }

  "toString" in {
    assert(AgentProcessId(123, 789).toString == "AgentProcessId(123-789)")
    assert(AgentProcessId(-123, 789).toString == "AgentProcessId(-123-789)")
  }

  "apply(number)" in {
    assert(AgentProcessId(123000000789L) == AgentProcessId(123, 789))
    assert(AgentProcessId(-123000000789L) == AgentProcessId(-123, 789))
  }

  "apply(string)" in {
    assert(AgentProcessId("123-789").string == "123-789")
    assert(AgentProcessId("-123-789").string == "-123-789")
    assert(AgentProcessId("123000000789").string == "123-789")
    assert(AgentProcessId("-123000000789").string == "-123-789")
    intercept[IllegalArgumentException] { AgentProcessId("") }
    intercept[IllegalArgumentException] { AgentProcessId("123-") }
    intercept[IllegalArgumentException] { AgentProcessId("-789-") }
  }
}
