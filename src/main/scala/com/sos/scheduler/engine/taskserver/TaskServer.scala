package com.sos.scheduler.engine.taskserver

import com.google.inject.Guice
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.minicom.comrpc.ByteMessageExecutor
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import com.sos.scheduler.engine.taskserver.configuration.inject.TaskServerModule

/**
 * @author Joacim Zschimmer
 */
class TaskServer(conf: StartConfiguration) extends HasCloser {

  private val injector = Guice.createInjector(new TaskServerModule)
  private val executor = injector.apply[ByteMessageExecutor]
  private val connection = new TcpConnection(conf.controllerAddress).registerCloseable

  override def close(): Unit = {
    // TODO run() beenden.
    super.close()
  }

  def connectWithScheduler(): Unit = {
    connection.connect()
  }

  def run(): Unit = {
    while (receiveAndProcessMessage()) {}
  }

  private def receiveAndProcessMessage(): Boolean =
    connection.receiveMessage() match {
      case Some(call) ⇒
        val (bytes, n) = executor.executeMessage(call)
        connection.sendMessage(bytes, n)
        true
      case None ⇒
        false
    }

  def kill(): Unit = ???
}

object TaskServer {
  def run(conf: StartConfiguration): Unit =
    autoClosing(new TaskServer(conf)) { taskServer ⇒
      taskServer.connectWithScheduler()
      taskServer.run()
    }
}
