package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.agent.commands.{CloseRemoteTask, CloseRemoteTaskResponse, RemoteTaskCommand, Response, StartRemoteTask, StartRemoteTaskResponse}
import com.sos.scheduler.engine.agent.common.ScalaConcurrentHashMap
import com.sos.scheduler.engine.agent.task.RemoteTaskHandler._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import java.net.InetSocketAddress
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class RemoteTaskHandler @Inject private(newRemoteTaskId: () ⇒ RemoteTaskId, newRemoteTask: StartConfiguration ⇒ RemoteTask) {

  private val taskRegister = new ScalaConcurrentHashMap[RemoteTaskId, RemoteTask] {
    override def default(id: RemoteTaskId) = throwUnknownTask(id)
  }

  def executeCommand(command: RemoteTaskCommand) = Future[Response] {
    command match {
      case StartRemoteTask(controllerTcpPort, usesApi, javaOptions, javaClasspath) ⇒
        val remoteTaskId = newRemoteTaskId()
        val startConfiguration = StartConfiguration(
          remoteTaskId,
          controllerAddress = new InetSocketAddress(LocalHostIpAddress, controllerTcpPort),
          usesApi = usesApi,
          javaOptions = javaOptions,
          javaClasspath = javaClasspath)
        val task = newRemoteTask(startConfiguration)
        assert(task.id == remoteTaskId)
        taskRegister += task.id → task
        task.start()
        StartRemoteTaskResponse(task.id)

      case CloseRemoteTask(remoteTaskId, kill) ⇒
        val task = taskRegister.remove(remoteTaskId) getOrElse throwUnknownTask(remoteTaskId)
        if (kill) tryKillTask(task)
        task.close()
        CloseRemoteTaskResponse
      }
  }
}

private object RemoteTaskHandler {
  private val LocalHostIpAddress = "127.0.0.1"
  private val logger = Logger(getClass)

  private def tryKillTask(task: RemoteTask) =
    try task.kill()
    catch { case NonFatal(t) ⇒ logger.warn(s"Kill $task failed: $t") }

  private def throwUnknownTask(taskId: RemoteTaskId) = throw new NoSuchElementException(s"Unknown Task '$taskId'")
}
