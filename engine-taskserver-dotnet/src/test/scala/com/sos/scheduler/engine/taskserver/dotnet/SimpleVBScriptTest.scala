package com.sos.scheduler.engine.taskserver.dotnet

import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.taskserver.dotnet.SimpleDotnetTest.TestErrorMessage
import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleReference
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SimpleVBScriptTest extends SimpleDotnetTest {

  protected def language = "PowerShell"

  if (isWindows) {
    if (false) { // FIXME
    def vbscriptRef(script: String) = DotnetModuleReference.ScriptControl(language = "vbscript", script)

    addScriptErrorTest(vbscriptRef(s"""
      Function spooler_process
        Throw New Exception("$TestErrorMessage")
      End Function"""))

    addStandardTest(vbscriptRef("""
      Function spooler_process
        orderVariables = spooler_task.order.params
        v = spooler_task.order().params()
        value = orderVariables.value("TEST")
        if (value <> v.value("TEST")) Throw New Exception("order() and order are different")
        orderVariables.set_value("TEST", "TEST-CHANGED")
        spooler_log.log(0, value)
        spooler_process = True
      }"""))
  }
  }
}
