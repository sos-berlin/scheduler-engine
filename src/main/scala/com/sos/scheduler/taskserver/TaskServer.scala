package com.sos.scheduler.taskserver

import com.google.inject.Guice
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.taskserver.comrpc.ByteMessageExecutor
import com.sos.scheduler.taskserver.configuration.{StartConfiguration, TaskServerModule}

/**
 * @author Joacim Zschimmer
 */
private class TaskServer(conf: StartConfiguration) extends HasCloser {

  private val injector = Guice.createInjector(new TaskServerModule)
  private val executor = injector.apply[ByteMessageExecutor]
  private val connection = new TcpConnection(conf.controllerAddress).registerCloseable

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
}

object TaskServer {
  def run(conf: StartConfiguration): Unit =
    autoClosing(new TaskServer(conf)) { agent ⇒
      agent.connectWithScheduler()
      agent.run()
    }
}
