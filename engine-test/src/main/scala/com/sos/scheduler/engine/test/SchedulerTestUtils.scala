package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskEndedEvent, TaskId, TaskStartedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderState, OrderTouchedEvent}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, StartJobCommand}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem, Task, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.{ProcessClass, ProcessClassSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.TestSchedulerController.TestTimeout
import java.lang.System.currentTimeMillis
import java.nio.file.Files
import java.nio.file.Files.{exists, getLastModifiedTime}
import java.time.Instant.now
import java.util.concurrent.TimeoutException
import org.joda.time.{Duration, Instant}
import org.scalatest.Matchers._
import scala.collection.generic.CanBuildFrom
import scala.concurrent.{Await, ExecutionContext, Future}
import scala.language.{higherKinds, implicitConversions}
import scala.math.max
import scala.reflect.ClassTag
import scala.util.Try

object SchedulerTestUtils {

  private val logger = Logger(getClass)

  /**
   * Writes the configuration file and awaits JobScheduler's acceptance.
   */
  def deleteAndWriteConfigurationFile[A](path: TypedPath, xmlElem: xml.Elem)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit = {
    if (exists(controller.environment.fileFromPath(path))) {
      deleteConfigurationFile(path)
    }
    writeConfigurationFile(path, xmlElem)
  }

  /**
   * Writes the configuration file and awaits JobScheduler's acceptance.
   */
  def writeConfigurationFile[A](path: TypedPath, xmlElem: xml.Elem)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit = {
    val file = controller.environment.fileFromPath(path)
    if (exists(file) && getLastModifiedTime(file).toInstant.getEpochSecond == now().getEpochSecond) {
      logger.debug(s"Sleeping a second to get a different file time for $file")
      sleep(1.s)  // Assure different timestamp for configuration file, so JobScheduler can see a change
    }
    controller.eventBus.awaitingEvent[FileBasedEvent](e ⇒ e.key == path && (e.isInstanceOf[FileBasedAddedEvent] || e.isInstanceOf[FileBasedReplacedEvent])) {
      file.xml = xmlElem
      instance[FolderSubsystem].updateFolders()
    }
  }

  /**
   * Delete the configuration file and awaits JobScheduler's acceptance.
   */
  def deleteConfigurationFile[A](path: TypedPath)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit = {
    controller.eventBus.awaitingKeyedEvent[FileBasedRemovedEvent](path) {
      Files.delete(controller.environment.fileFromPath(path))
      instance[FolderSubsystem].updateFolders()
    }
  }

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

  def runJob(jobPath: JobPath)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout) = runJobAndWaitForEnd(jobPath)  // v1.10

  def runJobAndWaitForEnd(jobPath: JobPath)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): TaskResult =
    runJobAndWaitForEnd(jobPath, timeout.duration)

  def runJobAndWaitForEnd(jobPath: JobPath, timeout: Duration)(implicit controller: TestSchedulerController): TaskResult = {
    val run = runJobFuture(jobPath)
    awaitResult(run.result, timeout)
  }

  def runJobFuture(jobPath: JobPath, variables: Iterable[(String, String)] = Nil)(implicit controller: TestSchedulerController): TaskRun = {
    implicit val callQueue = controller.instance[SchedulerThreadCallQueue]
    inSchedulerThread { // All calls in JobScheduler Engine thread, to safely subscribe the events before their occurrence.
      val taskId = startJob(jobPath, variables = variables)
      val started = controller.eventBus.keyedEventFuture[TaskStartedEvent](taskId)
      val startedTime = started map { _ ⇒ currentTimeMillis() }
      val ended = controller.eventBus.keyedEventFuture[TaskEndedEvent](taskId)
      val endedTime = ended map { _ ⇒ currentTimeMillis() }
      val closed = controller.eventBus.keyedEventFuture[TaskClosedEvent](taskId)
      val result = for (_ ← closed; s ← startedTime; e ← endedTime)
                   yield TaskResult(jobPath, taskId, endedInstant = new Instant(e), duration = max(0, e - s).ms)
      TaskRun(jobPath, taskId, started, ended, closed, result)
    }
  }

  def startJob(jobPath: JobPath, variables: Iterable[(String, String)] = Nil)(implicit controller: TestSchedulerController): TaskId = {
    val response = controller.scheduler executeXml StartJobCommand(jobPath, variables = variables)
    TaskId((response.elem \ "answer" \ "ok" \ "task" \ "@id").toString().toInt)
  }

  final case class TaskRun(
    jobPath: JobPath,
    taskId: TaskId,
    started: Future[TaskStartedEvent],
    ended: Future[TaskEndedEvent],
    closed: Future[TaskClosedEvent],
    result: Future[TaskResult])

  final case class TaskResult(jobPath: JobPath, taskId: TaskId, endedInstant: Instant, duration: Duration) {
    def logString(implicit controller: TestSchedulerController): String = taskLog(taskId)
  }

  def runOrder(orderKey: OrderKey)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): OrderRunResult =
    awaitResult(startOrder(orderKey).result, timeout.duration)

  def startOrder(orderKey: OrderKey)(implicit controller: TestSchedulerController): OrderRun = startOrder(OrderCommand(orderKey))

  def startOrder(orderCommand: OrderCommand)(implicit controller: TestSchedulerController): OrderRun =
    OrderRun(orderCommand.orderKey) sideEffect { _ ⇒
      controller.scheduler executeXml orderCommand
    }

  final case class OrderRun(
    orderKey: OrderKey,
    touched: Future[OrderTouchedEvent],
    finished: Future[OrderFinishedEvent],
    result: Future[OrderRunResult])

  object OrderRun {
    def apply(orderKey: OrderKey)(implicit controller: TestSchedulerController): OrderRun = {
      val finished = controller.eventBus.keyedEventFuture[OrderFinishedEvent](orderKey)
      val touched = controller.eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
      val result = for (finishedEvent ← finished) yield OrderRunResult(orderKey, finishedEvent.state)
      OrderRun(orderKey, touched, finished, result)
    }
  }

  final case class OrderRunResult(orderKey: OrderKey, state: OrderState) {
    def logString(implicit controller: TestSchedulerController): String = orderLog(orderKey)
  }

  def taskLog(taskId: TaskId)(implicit controller: TestSchedulerController): String =
    ((controller.scheduler executeXml <show_task id={taskId.string} what="log"/>)
      .answer \ "task" \ "log").text

  def orderLog(orderKey: OrderKey)(implicit controller: TestSchedulerController): String =
    ((controller.scheduler executeXml <show_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} what="log"/>)
      .answer \ "order" \ "log").text

  implicit def executionContext(implicit hasInjector: HasInjector): ExecutionContext = instance[ExecutionContext]

  def awaitSuccess[A](f: Future[A])(implicit t: ImplicitTimeout): A = Await.ready(f, t.concurrentDuration).successValue

  def awaitFailure[A](f: Future[A])(implicit t: ImplicitTimeout): Throwable = awaitCompletion(f).failed.get

  def awaitCompletion[A](f: Future[A])(implicit t: ImplicitTimeout): Try[A] = Await.ready(f, t.concurrentDuration).value.get

  def awaitResults[A, M[X] <: TraversableOnce[X]](o: M[Future[A]])
      (implicit cbf: CanBuildFrom[M[Future[A]], A, M[A]], ec: ExecutionContext, timeout: ImplicitTimeout) =
    awaitResult(Future.sequence(o)(cbf, ec), TestTimeout)

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

  def interceptErrorLogEvent[A](errorCode: MessageCode)(body: ⇒ A)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): ErrorLogEvent =
    controller.toleratingErrorCodes(Set(errorCode)) {
      controller.eventBus.awaitingEvent[ErrorLogEvent](_.codeOption == Some(errorCode)) {
        body
      }
    }

  def interceptErrorLogEvents[A](errorCodes: Set[MessageCode])(body: ⇒ A)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit =
    controller.toleratingErrorCodes(errorCodes.toSet) {
      val futures = errorCodes map { o ⇒ controller.eventBus.eventFuture[ErrorLogEvent](_.codeOption == Some(o)) }
      body
      try awaitResults(futures)
      catch {
        case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for error messages $errorCodes")
      }
    }
}
