package com.sos.scheduler.engine.common.sprayutils

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class YamlPrinterTest extends FreeSpec {

  "YAML" in {
    val yaml = YamlPrinter(JsObject(
      "number" → JsNumber(1.23),
      "string" → JsString("STRING"),
      "array" → JsArray(
        JsNumber(1),
        JsNumber(2)),
      "nested" → JsObject(
        "a" → JsBoolean(true))))
    assert(yaml contains "number: 1.23\n")
    assert(yaml contains "string: STRING\n")
    assert(yaml contains "array:\n")
    assert(yaml contains "- 1\n")
    assert(yaml contains "- 2\n")
    assert(yaml contains "nested:\n")
    assert(yaml contains "  a: true\n")
  }
}
