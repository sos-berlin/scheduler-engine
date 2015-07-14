package com.sos.scheduler.engine.kernel.job.internal

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Futures.catchInFuture
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.kernel.job.internal.JavaJobSignatures.{SpoolerExitSignature, SpoolerOnErrorSignature, SpoolerOnSuccessSignature, SpoolerOpenSignature}
import scala.util.{Success, Try}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class InternalModule private(injector: Injector, moduleName: String, agentUri: String, task: Task) {

  private implicit val schedulerThreadCallQueue = injector.instance[SchedulerThreadCallQueue]
  private lazy val instance: AsynchronousJob = newInternalJobInstance(moduleName)(task)

  import schedulerThreadCallQueue.implicits.executionContext

  @ForCpp
  def addObj(idispatch: AnyRef, name: String): Unit = {}

  @ForCpp
  def nameExists(name: String) = false

  @ForCpp
  def start(cppCall: CppCall): Unit = catchInFuture { instance.start() } onComplete cppCall.call

  @ForCpp
  def begin(cppCall: CppCall): Unit = cppCall.call(Success(true))

  @ForCpp
  def step(cppCall: CppCall): Unit = catchInFuture { instance.step() } onComplete cppCall.call

  @ForCpp
  def call(name: String, cppCall: CppCall): Unit =
    cppCall.call(Try {
      name match {
        case SpoolerOpenSignature ⇒ true
        case SpoolerOnErrorSignature ⇒ ()
        case SpoolerOnSuccessSignature ⇒ ()
        case SpoolerExitSignature ⇒ ()
        case _ ⇒ throw new UnsupportedOperationException(name)
      }
    })

  @ForCpp
  def end(success: Boolean, cppCall: CppCall): Unit = catchInFuture { instance.end() } onComplete cppCall.call

  @ForCpp
  def close(cppCall: CppCall): Unit = cppCall.call(Try { instance.close() })

  @ForCpp
  def release(cppCall: CppCall): Unit = cppCall.call(Success(()))

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
