package com.sos.scheduler.engine.common.process.windows

import CommandLineConversion.argsToCommandLine
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class CommandLineConversionTest extends FreeSpec {

  "argsToCommandLine" in {
    assert(argsToCommandLine(List("./a", "b c")) == """.\a "b c"""")
    assert(argsToCommandLine(List("""C:\Program Files\x""", "b c")) == """"C:\Program Files\x" "b c"""")
    intercept[IllegalArgumentException] {
      argsToCommandLine(List("""a"b"""))
    }
    intercept[IllegalArgumentException] {
      argsToCommandLine(List("a", """a""""))
    }
  }
}
