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
      // StartTaskResponse serializes to JSON without $TYPE, then deserializes to StartTaskSucceed (renamed from StartTaskResponse)
      """{
        "agentTaskId": "1-2",
        "tunnelToken": {
          "id": "TUNNEL-ID",
          "secret": "SECRET"
        }
      }""".parseJson)
  }

  "StartTaskFailed" in {
    check(
      StartTaskFailed("FAILED"),
      """{
        "$TYPE": "StartTaskFailed",
        "message": "FAILED"
      }""".parseJson)
  }

  private def check(o: StartTaskResponse, js: JsValue): Unit = {
    assert(o.toJson == js)
    assert((o: Response).toJson == js)
    assert(js.convertTo[StartTaskResponse] == o)
  }
}
