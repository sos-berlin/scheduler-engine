package sos.spooler.jobs

import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.junit.runner.RunWith
import ScriptAdapterJob._
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class ScriptAdapterJobTest extends FunSuite {

  test("ScriptAdapterJob.parseLanguageParameter") {
    intercept[RuntimeException] { parseLanguageParameter("INVALID-PREFIX:language") }
    intercept[RuntimeException] { parseLanguageParameter("java") }
    intercept[RuntimeException] { parseLanguageParameter(":language") }
    parseLanguageParameter("java:language") should equal (new Parameters("language", true))
    parseLanguageParameter("java:") should equal (new Parameters("", true))
    parseLanguageParameter("javax.script:language") should equal (new Parameters("language", false))
  }
}
