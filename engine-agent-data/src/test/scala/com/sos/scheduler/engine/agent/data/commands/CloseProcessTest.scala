package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.AgentProcessId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class CloseProcessTest extends FreeSpec {

  "JSON" in {
    val obj = CloseProcess(AgentProcessId("111-222"), kill = true)
    val json = """{
      "$TYPE": "CloseProcess",
      "processId": "111-222",
      "kill": true
    }""".parseJson
    assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
    assert(obj == json.convertTo[Command])
  }
}
