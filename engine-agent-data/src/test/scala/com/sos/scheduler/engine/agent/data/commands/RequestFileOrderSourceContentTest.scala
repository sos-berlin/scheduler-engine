package com.sos.scheduler.engine.agent.data.commands

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RequestFileOrderSourceContentTest extends FreeSpec {

  "JSON" in {
    val obj = RequestFileOrderSourceContent(
      directory = "DIRECTORY",
      regex = "REGEX",
      durationMillis = 111222333444555666L,
      knownFiles = Set("KNOWN"))
    val json = """{
      "$TYPE": "RequestFileOrderSourceContent",
      "directory": "DIRECTORY",
      "regex": "REGEX",
      "durationMillis": 111222333444555666,
      "knownFiles": [ "KNOWN" ]
    }""".parseJson
    assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
    assert(obj == json.convertTo[Command])
  }
}
