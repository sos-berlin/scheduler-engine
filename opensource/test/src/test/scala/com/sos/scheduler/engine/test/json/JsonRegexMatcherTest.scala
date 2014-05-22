package com.sos.scheduler.engine.test.json

import JsonRegexMatcher._
import org.scalatest.FreeSpec

/**
 * @author Joacim Zschimmer
 */
final class JsonRegexMatcherTest extends FreeSpec {
  "checkRegexJson" in {
    val json = """{"a": 1, "b": "BB"}"""
    checkRegexJson(json, Map("a" -> 1, "b" -> "BB"))
    intercept[RuntimeException] { checkRegexJson(json, Map("a" -> 1)) }
    intercept[RuntimeException] { checkRegexJson(json, Map("a" -> 1, "b" -> 2, "c" -> 3)) }
    checkRegexJson(json, Map("a" -> AnyInt, "b" -> """B+""".r))
    intercept[RuntimeException] { checkRegexJson("""{"a": "XX"}""", Map("a" -> """A+""".r)) }
  }
}
