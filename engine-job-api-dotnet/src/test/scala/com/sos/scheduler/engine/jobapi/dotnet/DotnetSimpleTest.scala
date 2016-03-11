package com.sos.scheduler.engine.jobapi.dotnet

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.jobapi.dotnet.DotnetSimpleTest._
import com.sos.scheduler.engine.jobapi.dotnet.api.{DotnetModuleInstanceFactory, DotnetModuleReference, TaskContext}
import java.nio.file.Files
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
          val invoker = mock[sos.spooler.Invoker]
          val job = factory.newInstance(
            classOf[sos.spooler.Job_impl],
            newTaskContext(invoker),
            DotnetModuleReference.Powershell(PowershellScript))
          job.spooler_process()
          verify(invoker.call("info", Array("HELLO")))
        }
        env.directory
      }
    assert(Files.exists(dotnetDir))
  }
}

private object DotnetSimpleTest {
  private val PowershellScript = """
   |function spooler_process() {
   |  $value = $spooler_task.order.params.value("TEST")
   |  $spooler_log.info("HELLO")
   |  return true
   |}
   |""".stripMargin

  private def newTaskContext(invoker: sos.spooler.Invoker) = TaskContext(
    spoolerLog = new sos.spooler.Log(invoker),
    spoolerTask = new sos.spooler.Task(invoker),
    spoolerJob = new sos.spooler.Job(invoker),
    spooler = new sos.spooler.Spooler(invoker))
}
