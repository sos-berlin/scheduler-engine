package com.sos.scheduler.engine.taskserver.dotnet

import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.common.utils.Exceptions
import com.sos.scheduler.engine.common.utils.Exceptions.ignoreException
import com.sos.scheduler.engine.taskserver.dotnet.DotnetSimpleTest._
import com.sos.scheduler.engine.taskserver.dotnet.api.{DotnetModuleReference, TaskContext}
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import org.scalatest.{BeforeAndAfterAll, FreeSpec}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class DotnetSimpleTest extends FreeSpec with HasCloser with BeforeAndAfterAll {

  private lazy val dotnetEnvironment = new DotnetEnvironment(temporaryDirectory) sideEffect { o ⇒
    onClose {
      ignoreException(logger.debug) {
        o.close()
      }
    }
  }
  private lazy val instanceFactory = new Jni4netModuleInstanceFactory(dotnetEnvironment.directory).closeWithCloser

  override protected def afterAll() = closer.close()

  if (!isWindows) {
    ".Net is only for Windows" - {}
  } else {
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
          throw "$error"
        }"""))
      val e = intercept[Exception] {
        job.spooler_process()
      }
      assert(e.getMessage contains error)
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
          return $true
        }"""))
      job.spooler_process()
      verify(spoolerTaskInvoker, times(2)).call("<order", Array())   // The adapter's spooler_process does one extra call
      verify(orderInvoker).call("<params", Array())
      verify(paramsInvoker).call("<value", Array("TEST"))
      verify(spoolerLogInvoker).call("log", Array(0: Integer, "HELLO"))
    }
  }
}

private object DotnetSimpleTest {
  private val logger = Logger(getClass)
}
