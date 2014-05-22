package com.sos.scheduler.engine.test

import _root_.scala.concurrent.{Await, Future}
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, Order}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerException, HasInjector}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.Duration
import org.scalatest.Matchers._

object SchedulerTestUtils {

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    hasInjector.injector.getInstance(classOf[JobSubsystem]).job(jobPath)

  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).jobChain(jobChainPath)

  def order(key: OrderKey)(implicit hasInjector: HasInjector): Order =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).order(key)

  def orderOption(key: OrderKey)(implicit hasInjector: HasInjector): Option[Order] =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).orderOption(key)

  def runJobAndWaitForEnd(jobPath: JobPath)(implicit controller: TestSchedulerController, timeout: TestTimeout): TaskId = {
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
      Tuple2(taskId, future)
    }
  }

  def startJob(jobPath: JobPath)(implicit controller: TestSchedulerController): TaskId = {
    val response = controller.scheduler executeXml <start_job job={jobPath.string}/>
    TaskId((response.elem \ "answer" \ "ok" \ "task" \ "@id").toString().toInt)
  }

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
