package com.sos.scheduler.engine.taskserver

import com.google.inject.Guice
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.minicom.comrpc.ByteMessageExecutor
import com.sos.scheduler.engine.taskserver.SimpleTaskServer._
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import com.sos.scheduler.engine.taskserver.configuration.inject.TaskServerModule
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent._
import scala.concurrent.duration.Duration

/**
 * @author Joacim Zschimmer
 */
final class SimpleTaskServer(conf: StartConfiguration) extends TaskServer with HasCloser {

  private val injector = Guice.createInjector(new TaskServerModule)
  private val executor = injector.apply[ByteMessageExecutor]
  private val controllingScheduler = new TcpConnection(conf.controllerAddress).closeWithCloser

  private val terminatedPromise = Promise[Unit]()
  def terminated = terminatedPromise.future

  def start(): Unit =
    Future {
      blocking {
        controllingScheduler.connect()
        try while (processNextMessage()) {}
        catch {
          case t: Throwable ⇒
            logger.error(t.toString, t)
            throw t
        }
      }
    } onComplete { o ⇒
      terminatedPromise.complete(o)
    }

  private def processNextMessage(): Boolean =
    controllingScheduler.receiveMessage() match {
      case Some(callBytes) ⇒
        val (resultBytes, n) = executor.executeMessage(controllingScheduler, callBytes)
        controllingScheduler.sendMessage(resultBytes, n)
        true
      case None ⇒
        false
    }

  def kill(): Unit = controllingScheduler.close()

  override def toString = s"TaskServer(controller=${conf.controllerAddress})"
}

object SimpleTaskServer {
  private val logger = Logger(getClass)

  def run(conf: StartConfiguration): Unit =
    autoClosing(new SimpleTaskServer(conf)) { taskServer ⇒
      taskServer.start()
      Await.result(taskServer.terminated, Duration.Inf)
    }
}
