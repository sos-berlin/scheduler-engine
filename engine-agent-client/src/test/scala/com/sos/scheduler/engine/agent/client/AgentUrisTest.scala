package com.sos.scheduler.engine.agent.client

import com.sos.scheduler.engine.tunnel.data.TunnelId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentUrisTest extends FreeSpec {

  private val agentUris = AgentUris("http://example.com:9999")

  "command" in {
    assert(agentUris.command ==
      "http://example.com:9999/jobscheduler/agent/command")
  }

  "tunnel" in {
    assert(agentUris.tunnel(TunnelId(", &+/%00")) ==
      "http://example.com:9999/jobscheduler/agent/tunnel/,%20&+%2F%2500")
  }

  "fileStatus" in {
    assert(agentUris.fileStatus("/FILE X+") ==
      "http://example.com:9999/jobscheduler/agent/fileStatus?file=/FILE+X%2B")
  }
}
