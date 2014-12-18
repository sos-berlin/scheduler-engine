package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.agent.task.InProcessRemoteTask._
import com.sos.scheduler.engine.agent.task.RemoteTaskId
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.taskserver.TaskServer
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent._

/**
 * @author Joacim Zschimmer
 */
final class InProcessRemoteTask(val id: RemoteTaskId, val configuration: StartConfiguration) extends RemoteTask {

  private val taskServer = new TaskServer(configuration)
  
  def connectWithScheduler() = taskServer.connectWithScheduler()

  def start(): Unit = {
    Future {
      blocking {
        try taskServer.run()
        catch {
          case t: Throwable ⇒
            logger.error(t.toString, t)
            throw t
        }
      }
    }
  }

  def kill(): Unit = {
    taskServer.kill()
  }

  def close(): Unit = {
    taskServer.close()
  }
}

private object InProcessRemoteTask {
  private val logger = Logger(getClass)
}
