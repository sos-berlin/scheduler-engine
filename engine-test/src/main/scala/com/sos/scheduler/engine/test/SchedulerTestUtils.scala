package com.sos.scheduler.engine.test

import _root_.scala.collection.generic.CanBuildFrom
import _root_.scala.concurrent.{Await, ExecutionContext, Future}
import _root_.scala.language.higherKinds
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem, Task, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.{ProcessClass, ProcessClassSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.TestSchedulerController.TestTimeout
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.Duration

object SchedulerTestUtils {

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    hasInjector.injector.apply[JobSubsystem].job(jobPath)

  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    hasInjector.injector.apply[OrderSubsystem].jobChain(jobChainPath)

  def order(key: OrderKey)(implicit hasInjector: HasInjector): Order =
    hasInjector.injector.apply[OrderSubsystem].order(key)

  def orderOption(key: OrderKey)(implicit hasInjector: HasInjector): Option[Order] =
    hasInjector.injector.apply[OrderSubsystem].orderOption(key)

  def task(taskId: TaskId)(implicit hasInjector: HasInjector): Task =
    hasInjector.injector.apply[TaskSubsystem].task(taskId)

  def processClass(path: ProcessClassPath)(implicit hasInjector: HasInjector): ProcessClass =
    hasInjector.injector.apply[ProcessClassSubsystem].processClass(path)

  def runJobAndWaitForEnd(jobPath: JobPath)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): TaskId = {
    runJobAndWaitForEnd(jobPath, timeout.duration)
  }

  def runJobAndWaitForEnd(jobPath: JobPath, timeout: Duration)(implicit controller: TestSchedulerController): TaskId = {
    val (taskId, future) = runJobFuture(jobPath)
    Await.result(future, timeout)
    taskId
  }

  def runJobFuture(jobPath: JobPath)(implicit controller: TestSchedulerController): (TaskId, Future[TaskClosedEvent]) = {
    implicit val callQueue = controller.injector.apply[SchedulerThreadCallQueue]
    inSchedulerThread {
      val taskId = startJob(jobPath)
      // Im selben Thread, damit wir sicher das Event abonnieren, bevor es eintrifft. Sonst ginge es verloren.
      val future = controller.getEventBus.keyedEventFuture[TaskClosedEvent](taskId)
      (taskId, future)
    }
  }

  def startJob(jobPath: JobPath)(implicit controller: TestSchedulerController): TaskId = {
    val response = controller.scheduler executeXml <start_job job={jobPath.string}/>
    TaskId((response.elem \ "answer" \ "ok" \ "task" \ "@id").toString().toInt)
  }

  implicit def executionContext(implicit hasInjector: HasInjector): ExecutionContext = hasInjector.injector.apply[ExecutionContext]

  def awaitResult[A](o: Future[A])(implicit timeout: ImplicitTimeout) = Await.result(o, timeout.concurrentDuration)

  def awaitResults[A, M[X] <: TraversableOnce[X]](o: M[Future[A]])
      (implicit cbf: CanBuildFrom[M[Future[A]], A, M[A]], ec: ExecutionContext, timeout: ImplicitTimeout) =
    Await.result(Future.sequence(o)(cbf, ec), TestTimeout)

//  /** Fängt eine Exception ab, die auch vom JobScheduler als Fehlermeldung ins Hauptprotokoll geschrieben wird.
//    * Eine Fehlermeldung im Hauptprotokoll führt gewöhnlich zum Abbruch des Tests.
//    * @param errorCode Code der im Hauptprokoll zu tolerierenden Fehlermeldung.
//    * @param testException: Test-Code, der bei einer falschen CppException eine Exception wirft, zum Bespiel mit ScalaTest. */
//  def interceptLoggedSchedulerError(errorCode: MessageCode, testException: SchedulerException ⇒ Unit = _ ⇒ ())(body: ⇒ Unit)(implicit controller: TestSchedulerController) {
//    controller.toleratingErrorLogEvent(errorCode) {
//      val e = intercept[SchedulerException](body)
//      s"${e.getMessage} " should startWith (s"$errorCode ")
//      testException(e)
//    }
//  }
}
