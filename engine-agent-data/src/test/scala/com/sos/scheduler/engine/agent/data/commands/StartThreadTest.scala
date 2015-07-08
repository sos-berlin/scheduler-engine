package com.sos.scheduler.engine.agent.data.commands

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class StartThreadTest extends FreeSpec {

  "JSON" in {
    val obj = StartThread(controllerAddressOption = Some("CONTROLLER:7"))
    val json = """{
      "$TYPE": "StartThread",
      "controllerAddressOption": "CONTROLLER:7"
    }""".parseJson
    assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
    assert(obj == json.convertTo[Command])
  }
}
