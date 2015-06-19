package com.sos.scheduler.engine.agent.client

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentUrisTest extends FreeSpec {

  "AgentUris" in {
    val agentUris = AgentUris("http://example.com:9999")
    assert(agentUris.command ==
      "http://example.com:9999/jobscheduler/agent/command")
    assert(agentUris.fileStatus("/FILE X+") ==
      "http://example.com:9999/jobscheduler/agent/fileStatus?file=/FILE+X%2B")
  }
}
