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
          val spoolerLog, spoolerTask, order, params = mock[sos.spooler.Invoker]
          when(spoolerTask.call("<order", Array())).thenReturn(order, null)
          when(order.call("<params", Array())).thenReturn(params, null)
          when(params.call("<value", Array("TEST"))).thenReturn("HELLO", null)
          val job = factory.newInstance(
            classOf[sos.spooler.Job_impl],
            TaskContext(
              new sos.spooler.Log(spoolerLog),
              new sos.spooler.Task(spoolerTask),
              new sos.spooler.Job(null/*not used*/),
              new sos.spooler.Spooler(null/*not used*/)),
            DotnetModuleReference.Powershell(PowershellScript))
          job.spooler_process()
          verify(spoolerLog).call("log", Array(0: Integer, "HELLO"))
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
