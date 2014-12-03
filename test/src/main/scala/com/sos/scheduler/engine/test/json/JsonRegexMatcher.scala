package com.sos.scheduler.engine.test.json

import org.scalatest.Matchers._
import scala.util.matching.Regex
import spray.json._

/**
 * @author Joacim Zschimmer
 */
object JsonRegexMatcher {
  val AnyIsoTimestamp = """\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d.\d\d\dZ""".r
  case object AnyInt

  /** Wirft eine Exception, falls json nicht dem Muster entspricht. */
  def checkRegexJson(json: String, patternMap: Map[String, Any]): Unit = {
    val jsObject = json.parseJson.asJsObject
    patternMap.keySet shouldEqual jsObject.fields.keySet
    for ((name, expectedValue) <- patternMap) {
      withClue(s"JSON field $name:") {
        (jsObject.fields(name), expectedValue) match {
          case (JsString(string), regex: Regex) ⇒
            string should fullyMatch regex regex
          case (JsNumber(n), AnyInt) ⇒
            if (!n.isValidInt) fail(s"Not an Int: $n")
          case (JsString(string), expected: String) if string == expected ⇒
          case (JsNumber(number), expected: Number) if number == expected ⇒
          case (expected, jsonValue) if jsonValue != expected ⇒
            fail(s"Not as expected: json=$jsonValue, expected=$expected")
        }
      }
    }
  }
}
