package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.AgentProcessId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ProcessDescriptorTest extends FreeSpec {
  "fromXml" in {
    ProcessDescriptor.fromXml(<spooler><answer><process process_id="111222333444555666" pid="2222"/></answer></spooler>.toString()) shouldEqual
      ProcessDescriptor(agentProcessId = AgentProcessId("111222333444555666"), pid = 2222)
  }
}
