package com.sos.scheduler.engine.kernel.job.internal

import com.google.inject.Injector
import com.sos.jobscheduler.agent.client.AgentClient
import com.sos.jobscheduler.agent.data.commands.{DeleteFile, MoveFile}
import com.sos.jobscheduler.base.utils.ScalaUtils.cast
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Futures.catchInFuture
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.kernel.job.internal.FileOrderSinkJob._
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.SinkNode
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystemClient
import java.nio.file.Files._
import java.nio.file.StandardCopyOption.REPLACE_EXISTING
import java.nio.file.{Files, Paths}
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.util.Try

/**
 * Implementation of &lt;file_order_sink>.
 *
 * @author Joacim Zschimmer
 */
final class FileOrderSinkJob private(
  protected val task: Task,
  protected val messageCodeToString: MessageCodeHandler,
  processClassSubsystem: ProcessClassSubsystemClient,
  agentClientFactory: SchedulerAgentClientFactory)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
   executionContext: ExecutionContext)
extends StandardAsynchronousJob with OrderAsynchronousJob {

  import task.log

  protected def processFileOrder(order: Order): Future[Boolean] = {
    val filePath = order.filePath
    val fileOperator = {
      val processClassOption = order.jobChain.fileWatchingProcessClassPathOption map processClassSubsystem.processClass
      processClassOption flatMap { _.agents.headOption } match {
        case Some(agent) ⇒ new AgentFileOperator(agentClientFactory.apply(agent.address))
        case None ⇒ LocalFileOperator
      }
    }
    val node = cast[SinkNode](order.jobChain.node(order.nodeId))

    val resultFuture: Future[Boolean] = catchInFuture {
      (node.moveFileTo, node.isDeletingFile) match {
        case ("", true) ⇒
          log.info(messageCodeToString(MessageCode("SCHEDULER-979"), filePath))
          fileOperator.deleteFile(filePath)
        case (destination, false) ⇒
          log.info(messageCodeToString(MessageCode("SCHEDULER-980"), filePath, destination))
          fileOperator.moveFile(filePath, toDirectory = destination)
        case _ ⇒ throw new IllegalArgumentException
      }
    }
    .map { _ ⇒ true }
    .recoverWith { case t ⇒
      log.warn(t.toString)
      fileOperator.fileExists(filePath) flatMap { exists ⇒
        schedulerThreadFuture {
          if (exists) order.setOnBlacklist()
          !exists
        }
      }
    }

    val promise = Promise[Boolean]()
    resultFuture.onComplete { result: Try[Boolean] ⇒
      promise.completeWith {
        schedulerThreadFuture { order.setEndStateReached() } map { _ ⇒ result.get }    // JS-1627 <file_order_sink> must not changed order nodeId
      }
    }
    promise.future
  }
}

object FileOrderSinkJob {
  def apply(injector: Injector)(task: Task) =
    new FileOrderSinkJob(
      task,
      injector.instance[MessageCodeHandler],
      injector.instance[ProcessClassSubsystemClient],
      injector.instance[SchedulerAgentClientFactory])(
        injector.instance[SchedulerThreadCallQueue],
        injector.instance[ExecutionContext])

  private trait FileOperator {
    def deleteFile(path: String): Future[Unit]
    def moveFile(path: String, toDirectory: String): Future[Unit]
    def fileExists(path: String): Future[Boolean]
  }

  private object LocalFileOperator extends FileOperator {
    import Files.{delete, exists, move}
    def deleteFile(path: String) = Future.successful(delete(Paths.get(path)))
    def moveFile(pathString: String, toDirectoryString: String) = {
      // Code is duplicate with Agent FileCommandExecutor !!!
      val path = Paths.get(pathString)
      val toDirectory = Paths.get(toDirectoryString)
      require(isDirectory(toDirectory), s"Move destination is not a directory: $toDirectory")
      move(path, toDirectory resolve path.getFileName, REPLACE_EXISTING)
      Future.successful(())
    }
    def fileExists(path: String) = Future.successful(exists(Paths.get(path)))
  }

  private class AgentFileOperator(agentClient: AgentClient)(implicit ec: ExecutionContext) extends FileOperator {
    def deleteFile(path: String) = agentClient.executeCommand(DeleteFile(path)) map { _ ⇒ () }
    def moveFile(path: String, toDirectory: String) = agentClient.executeCommand(MoveFile(path, toDirectory)) map { _ ⇒ () }
    def fileExists(path: String) = agentClient.fileExists(path)
  }
}
