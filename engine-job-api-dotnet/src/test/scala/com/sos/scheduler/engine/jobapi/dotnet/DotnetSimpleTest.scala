package com.sos.scheduler.engine.jobapi.dotnet

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.jobapi.dotnet.DotnetSimpleTest._
import com.sos.scheduler.engine.jobapi.dotnet.api.{DotnetModuleInstanceFactory, DotnetModuleReference, TaskContext}
import java.nio.file.Files.exists
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class DotnetSimpleTest extends FreeSpec {

  "PowerShell calls spooler_log.info" in {
    val dotnetDir =
      autoClosing(new DotnetEnvironment(temporaryDirectory)) { env ⇒
        autoClosing(new Jni4netModuleInstanceFactory(env.directory)) { factory: DotnetModuleInstanceFactory ⇒
          val spoolerLogInvoker, spoolerTaskInvoker, orderInvoker, paramsInvoker = mock[sos.spooler.Invoker]
          val order = new sos.spooler.Order(orderInvoker)
          val variableSet = new sos.spooler.Variable_set(paramsInvoker)
          when(spoolerTaskInvoker.call("<order", Array())).thenReturn(order, null)
          when(orderInvoker.call("<params", Array())).thenReturn(variableSet, null)
          when(paramsInvoker.call("<value", Array("TEST"))).thenReturn("HELLO", null)
          val job = factory.newInstance(
            classOf[sos.spooler.Job_impl],
            TaskContext(
              new sos.spooler.Log(spoolerLogInvoker),
              new sos.spooler.Task(spoolerTaskInvoker),
              new sos.spooler.Job(null/*not used*/),
              new sos.spooler.Spooler(null/*not used*/)),
            DotnetModuleReference.Powershell(PowershellScript))
          job.spooler_process()
          verify(spoolerLogInvoker).call("log", Array(0: Integer, "HELLO"))
        }
        env.directory
      }
    assert(exists(dotnetDir))
  }
}

private object DotnetSimpleTest {
  private val PowershellScript = """
    function spooler_process() {
      $value = $spooler_task.order.params.value("TEST")
      $spooler_log.log(0, $value)
      return true
    }"""
}
