package com.sos.scheduler.engine.jobapi.dotnet

import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.jobapi.dotnet.api.{DotnetModuleReference, TaskContext}
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import org.scalatest.{BeforeAndAfterAll, FreeSpec}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class DotnetSimpleTest extends FreeSpec with BeforeAndAfterAll {

  private lazy val dotnetEnvironment = new DotnetEnvironment(temporaryDirectory)
  private lazy val instanceFactory = new Jni4netModuleInstanceFactory(dotnetEnvironment.directory)

  override protected def afterAll() = {
    instanceFactory.close()
    dotnetEnvironment.close()
  }

  "Error in PowerShell script is detected" in {
    val spoolerTaskInvoker, orderInvoker = mock[sos.spooler.Invoker]
    val order = new sos.spooler.Order(orderInvoker)
    when(spoolerTaskInvoker.call("<order", Array())).thenReturn(order, null)
    val error = "POWERSHELL-ERROR"
    val taskContext = TaskContext(
      new sos.spooler.Log(null/*not used*/),
      new sos.spooler.Task(spoolerTaskInvoker),
      new sos.spooler.Job(null/*not used*/),
      new sos.spooler.Spooler(null/*not used*/))
    val job = instanceFactory.newInstance(classOf[sos.spooler.Job_impl], taskContext, DotnetModuleReference.Powershell(s"""
      function spooler_process() {
        throw "my_error"
      }"""))
    val e = intercept[Exception] {
      job.spooler_init()
      job.spooler_process()
    }
    assert(e.toString().contains("my_error"))
  }

  "PowerShell calls spooler_log.info" in {
    val spoolerLogInvoker, spoolerTaskInvoker, orderInvoker, paramsInvoker = mock[sos.spooler.Invoker]
    val order = new sos.spooler.Order(orderInvoker)
    val variableSet = new sos.spooler.Variable_set(paramsInvoker)
    when(spoolerTaskInvoker.call("<order", Array())).thenReturn(order, null)
    when(orderInvoker.call("<params", Array())).thenReturn(variableSet, null)
    when(paramsInvoker.call("<value", Array("TEST"))).thenReturn("HELLO", null)
    val taskContext = TaskContext(
      new sos.spooler.Log(spoolerLogInvoker),
      new sos.spooler.Task(spoolerTaskInvoker),
      new sos.spooler.Job(null/*not used*/),
      new sos.spooler.Spooler(null/*not used*/))
    val job = instanceFactory.newInstance(classOf[sos.spooler.Job_impl], taskContext, DotnetModuleReference.Powershell("""
      function spooler_process() {
        $value = $spooler_task.order().params().value("TEST")
        $spooler_log.log(0, $value)
        return true
      }"""))
    job.spooler_init()
    job.spooler_process()
    verify(spoolerTaskInvoker).call("<order", Array())
    verify(orderInvoker).call("<params", Array())
    verify(paramsInvoker).call("<value", Array("TEST"))
    verify(spoolerLogInvoker).call("log", Array(0: Integer, "HELLO"))
  }
}

private object DotnetSimpleTest {
}
