package com.sos.scheduler.engine.kernel.job.internal

import com.google.inject.Injector
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Futures.catchInFuture
import com.sos.jobscheduler.data.jobapi.JavaJobSignatures.{SpoolerExitSignature, SpoolerOnErrorSignature, SpoolerOnSuccessSignature, SpoolerOpenSignature}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.job.Task
import scala.concurrent.Future
import scala.util.{Success, Try}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class InternalModule private(injector: Injector, moduleName: String, agentUri: String, task: Task) {

  private val schedulerThreadExecutionContext = injector.instance[SchedulerThreadCallQueue].executionContext
  private lazy val instance: AsynchronousJob = newInternalJobInstance(moduleName)(task)

  @ForCpp
  def addObj(idispatch: AnyRef, name: String): Unit = {}

  @ForCpp
  def nameExists(name: String) = false

  @ForCpp
  def start(resultCall: CppCall): Unit = handleCall(resultCall) { instance.start() }

  @ForCpp
  def begin(resultCall: CppCall): Unit = resultCall.call(Success(true))

  @ForCpp
  def step(resultCall: CppCall): Unit = handleCall(resultCall) { instance.step() }

  @ForCpp
  def call(name: String, resultCall: CppCall): Unit =
    resultCall.call(Try {
      name match {
        case SpoolerOpenSignature ⇒ true
        case SpoolerOnErrorSignature ⇒ ()
        case SpoolerOnSuccessSignature ⇒ ()
        case SpoolerExitSignature ⇒ ()
        case _ ⇒ throw new UnsupportedOperationException(name)
      }
    })

  @ForCpp
  def end(success: Boolean, resultCall: CppCall): Unit = handleCall(resultCall) { instance.end() }

  @ForCpp
  def close(cppCall: CppCall): Unit = cppCall.call(Try { instance.close() })

  @ForCpp
  def release(resultCall: CppCall): Unit = resultCall.call(Success(()))

  private def handleCall[A](resultCall: CppCall)(body: ⇒ Future[A]): Unit =
    catchInFuture { body } .onComplete { result: Try[A] ⇒ resultCall.call(result) } (schedulerThreadExecutionContext)

  private def newInternalJobInstance(name: String)(task: Task) =
    name match {
      case "FileOrderSink" ⇒ FileOrderSinkJob(injector)(task)
    }
}

object InternalModule {
  @ForCpp
  def apply(injector: Injector, moduleName: String, agentUri: String, task: Task) =
    new InternalModule(injector, moduleName = moduleName, agentUri = agentUri, task)
}
