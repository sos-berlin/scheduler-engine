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

  "fileStatus" in {
    assert(agentUris.fileStatus("/FILE X+") ==
      "http://example.com:9999/jobscheduler/agent/fileStatus?file=/FILE+X%2B")
  }

  "tunnelHandler" - {
    "overview" in {
      assert(agentUris.tunnelHandler.overview ==
        "http://example.com:9999/jobscheduler/agent/tunnels")
    }

    "items" in {
      assert(agentUris.tunnelHandler.items ==
      "http://example.com:9999/jobscheduler/agent/tunnels/item")
    }

    "item" in {
      assert(agentUris.tunnelHandler.item(TunnelId("TUNNEL-ID")) ==
        "http://example.com:9999/jobscheduler/agent/tunnels/item/TUNNEL-ID")
    }
  }

  "overview" in {
    assert(agentUris.overview ==
      "http://example.com:9999/jobscheduler/agent/overview")
  }
}
