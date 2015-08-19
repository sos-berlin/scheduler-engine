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

  private val agentUris = AgentUris("http://example.com:9999/testPrefix")

  "command" in {
    assert(agentUris.command ==
      "http://example.com:9999/testPrefix/jobscheduler/agent/api/command")
  }

  "fileStatus" in {
    assert(agentUris.fileStatus("/FILE X+") ==
      "http://example.com:9999/testPrefix/jobscheduler/agent/api/fileStatus?file=/FILE+X%2B")
  }

  "tunnelHandler" - {
    "overview" in {
      assert(agentUris.tunnelHandler.overview ==
        "http://example.com:9999/testPrefix/jobscheduler/agent/api/tunnel")
    }

    "tunnels" in {
      assert(agentUris.tunnelHandler.tunnels ==
      "http://example.com:9999/testPrefix/jobscheduler/agent/api/tunnel/")
    }

    "tunnel" in {
      assert(agentUris.tunnelHandler.tunnel(TunnelId("TUNNEL-ID")) ==
        "http://example.com:9999/testPrefix/jobscheduler/agent/api/tunnel/TUNNEL-ID")
    }
  }

  "overview" in {
    assert(agentUris.overview ==
      "http://example.com:9999/testPrefix/jobscheduler/agent/api")
  }
}
