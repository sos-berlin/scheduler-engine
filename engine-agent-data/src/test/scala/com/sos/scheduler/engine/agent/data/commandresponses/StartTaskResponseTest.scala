package com.sos.scheduler.engine.agent.data.commandresponses

import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.tunnel.data.{TunnelId, TunnelToken}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class StartTaskResponseTest extends FreeSpec {

  "StartTaskSucceeded" in {
    check(
      StartTaskSucceeded(AgentTaskId("1-2"), TunnelToken(TunnelId("TUNNEL-ID"), TunnelToken.Secret("SECRET"))),
      """{
        "$TYPE": "StartTaskSucceeded",
        "agentTaskId": "1-2",
        "tunnelToken": {
          "id": "TUNNEL-ID",
          "secret": "SECRET"
        }
      }""")
  }

  "StartTaskFailed" in {
    check(
      StartTaskFailed("FAILED"),
      """{
        "$TYPE": "StartTaskFailed",
        "message": "FAILED"
      }""")
  }

  private def check(o: StartTaskResponse, json: String): Unit = {
    assert(o.toJson == json.parseJson)
    assert((o: Response).toJson == json.parseJson)
    assert(json.parseJson.convertTo[StartTaskResponse] == o)
  }
}
