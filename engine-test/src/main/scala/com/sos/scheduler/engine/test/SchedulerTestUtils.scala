package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem, Task, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.{ProcessClass, ProcessClassSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.TestSchedulerController.TestTimeout
import org.joda.time.Duration
import org.scalatest.Matchers._
import scala.collection.generic.CanBuildFrom
import scala.concurrent.{Await, ExecutionContext, Future}
import scala.language.{higherKinds, implicitConversions}
import scala.reflect.ClassTag
import scala.util.Try

object SchedulerTestUtils {

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    instance[JobSubsystem].job(jobPath)

  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    instance[OrderSubsystem].jobChain(jobChainPath)

  def order(key: OrderKey)(implicit hasInjector: HasInjector): Order =
    instance[OrderSubsystem].order(key)

  def orderOption(key: OrderKey)(implicit hasInjector: HasInjector): Option[Order] =
    instance[OrderSubsystem].orderOption(key)

  def task(taskId: TaskId)(implicit hasInjector: HasInjector): Task =
    instance[TaskSubsystem].task(taskId)

  def processClass(path: ProcessClassPath)(implicit hasInjector: HasInjector): ProcessClass =
    instance[ProcessClassSubsystem].processClass(path)

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
      // Im selben Thread, damit wir sicher das Event abonnieren, bevor es eintrifft. Sonst könnte es verlorengehen
      val future = controller.eventBus.keyedEventFuture[TaskClosedEvent](taskId)
      (taskId, future)
    }
  }

  def startJob(jobPath: JobPath)(implicit controller: TestSchedulerController): TaskId = {
    val response = controller.scheduler executeXml <start_job job={jobPath.string}/>
    TaskId((response.elem \ "answer" \ "ok" \ "task" \ "@id").toString().toInt)
  }

  implicit def executionContext(implicit hasInjector: HasInjector): ExecutionContext = instance[ExecutionContext]

  def awaitSuccess[A](f: Future[A])(implicit t: ImplicitTimeout): A = awaitCompletion(f).get

  def awaitFailure[A](f: Future[A])(implicit t: ImplicitTimeout): Throwable = awaitCompletion(f).failed.get

  def awaitCompletion[A](f: Future[A])(implicit t: ImplicitTimeout): Try[A] = Await.ready(f, t.concurrentDuration).value.get

  def awaitResults[A, M[X] <: TraversableOnce[X]](o: M[Future[A]])
      (implicit cbf: CanBuildFrom[M[Future[A]], A, M[A]], ec: ExecutionContext, timeout: ImplicitTimeout) =
    Await.result(Future.sequence(o)(cbf, ec), TestTimeout)

  def instance[A : ClassTag](implicit hasInjector: HasInjector): A = hasInjector.injector.getInstance(implicitClass[A])

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

  def interceptSchedulerError(errorCode: MessageCode)(body: ⇒ Unit)(implicit controller: TestSchedulerController): SchedulerException = {
    val result = intercept[SchedulerException] {
      controller.toleratingErrorCodes(Set(errorCode)) {
        body
      }
    }
    result.getMessage should startWith(errorCode.string)
    result
  }
}
