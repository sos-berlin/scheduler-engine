package com.sos.scheduler.engine.taskserver.dotnet

import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.common.utils.Exceptions.ignoreException
import com.sos.scheduler.engine.taskserver.dotnet.SimpleDotnetTest._
import com.sos.scheduler.engine.taskserver.dotnet.api.{DotnetModuleReference, TaskContext}
import org.mockito.Mockito._
import org.scalatest.mock.MockitoSugar.mock
import org.scalatest.{BeforeAndAfterAll, FreeSpec}

/**
  * @author Joacim Zschimmer
  */
trait SimpleDotnetTest extends FreeSpec with HasCloser with BeforeAndAfterAll {

  protected def language: String

  private lazy val dotnetEnvironment = new DotnetEnvironment(temporaryDirectory) sideEffect { o ⇒
    onClose {
      ignoreException(logger.debug) {
        o.close()
      }
    }
  }
  private lazy val instanceFactory = new Jni4netModuleInstanceFactory(dotnetEnvironment.directory).closeWithCloser

  override protected def afterAll() = closer.close()

  protected def addScriptErrorTest(dotnetModuleReference: DotnetModuleReference): Unit = {
    s"Error in $language script is detected" in {
      val spoolerTaskInvoker, orderInvoker = mock[sos.spooler.Invoker]
      val order = new sos.spooler.Order(orderInvoker)
      when(spoolerTaskInvoker.call("<order", Array())).thenReturn(order, null)
      val taskContext = TaskContext(
        new sos.spooler.Log(null /*not used*/),
        new sos.spooler.Task(spoolerTaskInvoker),
        new sos.spooler.Job(null /*not used*/),
        new sos.spooler.Spooler(null /*not used*/))
      val job = instanceFactory.newInstance(classOf[sos.spooler.Job_impl], taskContext, dotnetModuleReference)
      val e = intercept[Exception] {
        job.spooler_process()
      }
      assert(e.getMessage contains TestErrorMessage)
    }
  }

  protected def addStandardTest(dotnetModuleReference: DotnetModuleReference): Unit = {
    s"$language calls spooler_log.info" in {
      val spoolerLogInvoker, spoolerTaskInvoker, orderInvoker, paramsInvoker = mock[sos.spooler.Invoker]
      val order = new sos.spooler.Order(orderInvoker)
      val variableSet = new sos.spooler.Variable_set(paramsInvoker)
      when(spoolerTaskInvoker.call("<order", Array())).thenReturn(order, null)
      when(orderInvoker.call("<params", Array())).thenReturn(variableSet, null)
      when(paramsInvoker.call("<value", Array("TEST"))).thenReturn("HELLO", null)
      val taskContext = TaskContext(
        new sos.spooler.Log(spoolerLogInvoker),
        new sos.spooler.Task(spoolerTaskInvoker),
        new sos.spooler.Job(null /*not used*/),
        new sos.spooler.Spooler(null /*not used*/))
      val job = instanceFactory.newInstance(classOf[sos.spooler.Job_impl], taskContext, dotnetModuleReference)
      job.spooler_process()
      verify(spoolerTaskInvoker, times(2)).call("<order", Array()) // The adapter's spooler_process does one extra call
      verify(orderInvoker).call("<params", Array())
      verify(paramsInvoker).call("<value", Array("TEST"))
      verify(spoolerLogInvoker).call("log", Array(0: Integer, "HELLO"))
    }
  }
}

private[dotnet] object SimpleDotnetTest {
  private val logger = Logger(getClass)
  private[dotnet] val TestErrorMessage = "TEST-ERROR"
}
