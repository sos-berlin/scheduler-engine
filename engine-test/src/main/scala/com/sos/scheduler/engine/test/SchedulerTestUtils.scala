package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.data.job._
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderFinished, OrderKey, OrderOverview, OrderStarted}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, StartJobCommand}
import com.sos.scheduler.engine.eventbus.EventSubscription
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystemClient, Task, TaskSubsystemClient}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystemClient}
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.processclass.{ProcessClass, ProcessClassSubsystemClient}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerException}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.TestSchedulerController.TestTimeout
import java.lang.System.currentTimeMillis
import java.nio.file.Files
import java.nio.file.Files.{exists, getLastModifiedTime}
import java.time.Instant.now
import java.time.{Duration, Instant}
import java.util.concurrent.TimeoutException
import javax.persistence.EntityManagerFactory
import org.scalatest.Matchers._
import scala.collection.generic.CanBuildFrom
import scala.concurrent.{Await, ExecutionContext, Future, Promise}
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
    updateFoldersWith(path) {
      file.xml = xmlElem
      instance[FolderSubsystemClient].updateFolders()
    }
  }

  def updateFoldersWith(path: TypedPath)(body: ⇒ Unit)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit =
    controller.eventBus.awaiting[FileBasedAddedOrReplaced](path) {
      body
      instance[FolderSubsystemClient].updateFolders()
    }

  /**
   * Delete the configuration file and awaits JobScheduler's acceptance.
   */
  def deleteConfigurationFile[A](path: TypedPath)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit = {
    controller.eventBus.awaiting[FileBasedRemoved.type](path) {
      Files.delete(controller.environment.fileFromPath(path))
      instance[FolderSubsystemClient].updateFolders()
    }
  }

  def jobOverview(path: JobPath)(implicit hasInjector: HasInjector): JobOverview =
    jobView[JobOverview](path)

  def jobView[V <: JobView: JobView.Companion](path: JobPath)(implicit hasInjector: HasInjector): V =
    instance[JobSubsystemClient].jobView[V](path)

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    instance[JobSubsystemClient].job(jobPath)

  @deprecated("Avoid direct access to C++ near objects")
  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    instance[OrderSubsystemClient].jobChain(jobChainPath)

  def jobChainOverview(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChainOverview =
    instance[OrderSubsystemClient].overview(jobChainPath)

  def jobChainDetailed(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChainDetailed =
    instance[OrderSubsystemClient].detailed(jobChainPath)

  def orderOverview(orderKey: OrderKey)(implicit hasInjector: HasInjector): OrderOverview =
    instance[OrderSubsystemClient].orderOverview(orderKey)

  def orderDetailed(orderKey: OrderKey)(implicit hasInjector: HasInjector): OrderDetailed =
    instance[OrderSubsystemClient].orderDetailed(orderKey)

  @deprecated("Avoid direct access to C++ near objects")
  def order(orderKey: OrderKey)(implicit hasInjector: HasInjector): Order =
    instance[OrderSubsystemClient].order(orderKey)

  def orderExists(orderKey: OrderKey)(implicit hasInjector: HasInjector): Boolean = orderOption(orderKey).isDefined

  @deprecated("Avoid direct access to C++ near objects")
  def orderOption(orderKey: OrderKey)(implicit hasInjector: HasInjector): Option[Order] =
    instance[OrderSubsystemClient].orderOption(orderKey)

  def taskOverview(taskId: TaskId)(implicit hasInjector: HasInjector): TaskOverview =
    instance[TaskSubsystemClient].taskOverview(taskId) await TestTimeout

  def taskDetailed(taskId: TaskId)(implicit hasInjector: HasInjector): TaskDetailed =
    instance[TaskSubsystemClient].taskDetailed(taskId) await TestTimeout

  @deprecated("Avoid direct access to C++ near objects")
  def task(taskId: TaskId)(implicit hasInjector: HasInjector): Task =
    instance[TaskSubsystemClient].task(taskId)

  @deprecated("Avoid direct access to C++ near objects")
  def processClass(path: ProcessClassPath)(implicit hasInjector: HasInjector): ProcessClass =
    instance[ProcessClassSubsystemClient].processClass(path)

  def runJob(jobPath: JobPath, variables: Iterable[(String, String)] = Nil)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): TaskResult =
    runJob(StartJobCommand(jobPath, variables))

  def runJob(startJobCommand: StartJobCommand)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): TaskResult = {
    val run = startJob(startJobCommand)
    awaitResult(run.result, timeout.duration)
  }

  def startJob(jobPath: JobPath, variables: Iterable[(String, String)] = Nil)(implicit controller: TestSchedulerController): TaskRun =
    startJob(StartJobCommand(jobPath, variables))

  def startJob(startJobCommand: StartJobCommand)(implicit controller: TestSchedulerController): TaskRun = {
    implicit val callQueue = controller.instance[SchedulerThreadCallQueue]
    inSchedulerThread { // All calls in JobScheduler Engine thread, to safely subscribe the events before their occurrence.
      val commandResult = controller.scheduler executeXml startJobCommand
      val taskId = TaskId((commandResult.elem \ "answer" \ "ok" \ "task" \ "@id").toString().toInt)
      val taskKey = TaskKey(startJobCommand.jobPath, taskId)
      val started = controller.eventBus.eventFuture[TaskStarted.type](taskKey)
      val startedTime = started map { _ ⇒ currentTimeMillis() }
      val ended = controller.eventBus.eventFuture[TaskEnded](taskKey)
      val endedTime = ended map { _ ⇒ currentTimeMillis() }
      val closed = controller.eventBus.eventFuture[TaskClosed.type](taskKey)
      val result = for (_ ← closed; s ← startedTime; end ← ended; e ← endedTime)
                   yield TaskResult(startJobCommand.jobPath, taskId, end.returnCode, endedInstant = Instant.ofEpochMilli(e), duration = max(0, e - s).ms)
      TaskRun(startJobCommand.jobPath, taskId, started, ended, closed, result)
    }
  }

  final case class TaskRun(
    jobPath: JobPath,
    taskId: TaskId,
    started: Future[TaskStarted.type],
    ended: Future[TaskEnded],
    closed: Future[TaskClosed.type],
    result: Future[TaskResult])
  {
    def logString(implicit controller: TestSchedulerController): String = taskLog(taskId)
  }

  final case class TaskResult(jobPath: JobPath, taskId: TaskId, returnCode: ReturnCode, endedInstant: Instant, duration: Duration) {
    def logString(implicit controller: TestSchedulerController): String = taskLog(taskId)
  }

  def runOrder(orderKey: OrderKey)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): OrderRunResult =
    awaitResult(startOrder(orderKey).result, timeout.duration)

  def runOrder(orderCommand: OrderCommand)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): OrderRunResult =
    awaitResult(startOrder(orderCommand).result, timeout.duration)

  def startOrder(orderKey: OrderKey)(implicit controller: TestSchedulerController): OrderRun = startOrder(OrderCommand(orderKey))

  def startOrder(orderCommand: OrderCommand)(implicit controller: TestSchedulerController): OrderRun =
    OrderRun(orderCommand.orderKey) sideEffect { _ ⇒
      controller.scheduler executeXml orderCommand
    }

  final case class OrderRun(
    orderKey: OrderKey,
    touched: Future[OrderStarted.type],
    finished: Future[OrderFinished],
    result: Future[OrderRunResult])

  object OrderRun {
    def apply(orderKey: OrderKey)(implicit controller: TestSchedulerController): OrderRun = {
      import controller.eventBus
      val whenTouched = eventBus.eventFuture[OrderStarted.type](orderKey)
      val whenFinished: Future[(OrderFinished, Map[String, String])] = {
        val promise = Promise[(OrderFinished, Map[String, String])]()
        lazy val subscription: EventSubscription = EventSubscription[OrderFinished] {
          case KeyedEvent(`orderKey`, event) ⇒
            eventBus.unsubscribeHot(subscription)
            promise.success((event, orderDetailed(orderKey).variables))
        }
        eventBus.subscribeHot(subscription)
        promise.future
      }
      val result = for ((finishedEvent, variables) ← whenFinished) yield OrderRunResult(orderKey, finishedEvent.nodeId, variables)
      val whenFinishedEvent = whenFinished map { _._1 }
      new OrderRun(orderKey, whenTouched, whenFinishedEvent, result)
    }
  }

  final case class OrderRunResult(orderKey: OrderKey, nodeId: NodeId, variables: Map[String, String]) {
    def logString(implicit controller: TestSchedulerController): String = orderLog(orderKey)
  }

  def taskLog(taskId: TaskId)(implicit controller: TestSchedulerController): String =
    ((controller.scheduler executeXml <show_task id={taskId.string} what="log"/>)
      .answer \ "task" \ "log").text

  def orderLog(orderKey: OrderKey)(implicit controller: TestSchedulerController): String =
    ((controller.scheduler executeXml <show_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} what="log"/>)
      .answer \ "order" \ "log").text

  implicit def executionContext(implicit hasInjector: HasInjector): ExecutionContext = instance[ExecutionContext]

  def awaitSuccess[A](f: Future[A])(implicit t: ImplicitTimeout): A = Await.ready(f, t.concurrentDuration.toCoarsest).successValue

  def awaitFailure[A](f: Future[A])(implicit t: ImplicitTimeout): Throwable = awaitCompletion(f).failed.get

  def awaitCompletion[A](f: Future[A])(implicit t: ImplicitTimeout): Try[A] = Await.ready(f, t.concurrentDuration.toCoarsest).value.get

  def awaitResults[A, M[X] <: TraversableOnce[X]](o: M[Future[A]])
      (implicit cbf: CanBuildFrom[M[Future[A]], A, M[A]], ec: ExecutionContext, timeout: ImplicitTimeout) =
    awaitResult(Future.sequence(o)(cbf, ec), TestTimeout)

  def instance[A : ClassTag](implicit hasInjector: HasInjector): A = hasInjector.injector.getInstance(implicitClass[A])

//  /** Fängt eine Exception ab, die auch vom JobScheduler als Fehlermeldung ins Hauptprotokoll geschrieben wird.
//    * Eine Fehlermeldung im Hauptprotokoll führt gewöhnlich zum Abbruch des Tests.
//    * @param errorCode Code der im Hauptprokoll zu tolerierenden Fehlermeldung.
//    * @param testException: Test-Code, der bei einer falschen CppException eine Exception wirft, zum Bespiel mit ScalaTest. */
//  def interceptLoggedSchedulerError(errorCode: MessageCode, testException: SchedulerException ⇒ Unit = _ ⇒ ())(body: ⇒ Unit)(implicit controller: TestSchedulerController) {
//    controller.toleratingErrorLogged(errorCode) {
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

  def interceptErrorLogged[A](errorCode: MessageCode)(body: ⇒ A)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): ResultAndEvent[A] = {
    val errorLoggedFuture = controller.eventBus.futureWhen[ErrorLogged] { _.event.codeOption contains errorCode }
    val result = controller.toleratingErrorCodes(Set(errorCode)) { body }
    val errorLogged = try awaitResult(errorLoggedFuture, timeout.duration)
      catch { case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for error message $errorCode") }
    ResultAndEvent(result, errorLogged)
  }

  def interceptErrorLoggeds[A](errorCodes: Set[MessageCode])(body: ⇒ A)(implicit controller: TestSchedulerController, timeout: ImplicitTimeout): Unit =
    controller.toleratingErrorCodes(errorCodes) {
      val futures = errorCodes map { o ⇒ controller.eventBus.futureWhen[ErrorLogged](_.event.codeOption contains o) }
      body
      try awaitResults(futures)
      catch {
        case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for error messages $errorCodes")
      }
    }

  def orderIsBlacklisted(orderKey: OrderKey)(implicit hasInjector: HasInjector, entityManagerFactory: EntityManagerFactory): Boolean =
    if (jobChainOverview(orderKey.jobChainPath).isDistributed)
      transaction { implicit entityManager ⇒
        instance[HibernateOrderStore].fetch(orderKey).isBlacklisted
      }
    else
      orderOverview(orderKey).isBlacklisted

  final case class ResultAndEvent[A](result: A, event: ErrorLogged)
}
