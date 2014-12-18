package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.agent.commands.{CloseRemoteTask, CloseRemoteTaskResponse, RemoteTaskCommand, Response, StartRemoteTask, StartRemoteTaskResponse}
import com.sos.scheduler.engine.agent.task.RemoteTaskId
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import java.net.InetSocketAddress
import javax.inject.{Inject, Singleton}
import scala.collection.mutable
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class RemoteTaskHandler @Inject private {

  private val remoteTasks = mutable.Map[RemoteTaskId, RemoteTask]()
  private val remoteTaskIdIterator = RemoteTaskId.newGenerator()

  //TODO Threadsicher? Actor vorschalten?
  def executeCommand(command: RemoteTaskCommand): Future[Response] = {
    command match {
      case StartRemoteTask(controllerTcpPort, usesApi, javaOptions, javaClassPath) ⇒
        require(!usesApi, "Remote API tasks are not yet implemented")
        val task = new InProcessRemoteTask(remoteTaskIdIterator.next(), StartConfiguration(controllerAddress = new InetSocketAddress("127.0.0.1", controllerTcpPort)))
        remoteTasks += task.id → task
        Future {
          task.connectWithScheduler()
          task.start()
          StartRemoteTaskResponse(task.id)
        }

      case CloseRemoteTask(remoteTaskId, kill) ⇒
        val task = remoteTasks(remoteTaskId)
        try if (kill) task.kill()
        finally task.close()
        remoteTasks(remoteTaskId).close()
        remoteTasks -= remoteTaskId
        Future.successful { CloseRemoteTaskResponse }
    }
  }
}

private object RemoteTaskHandler {
  private val logger = Logger(getClass)
}
