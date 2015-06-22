package com.sos.scheduler.engine.agent.main

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.data.commands.Terminate
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import scala.util.control.NonFatal

/**
 * JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
object AgentMain {
  private val logger = Logger(getClass)
  private val OnJavaShutdownSigkillProcessesAfter = 5.s
  private val ShutdownTimeout = OnJavaShutdownSigkillProcessesAfter + 2.s

  def main(args: Array[String]): Unit = {
    val conf = AgentConfiguration(args)
    try run(conf)
    catch { case NonFatal(t) ⇒
      println(s"AGENT TERMINATED DUE TO ERROR: $t")
      logger.error(t.toString, t)
      System.exit(1)
    }
  }

  def run(conf: AgentConfiguration): Unit =
    autoClosing(new Agent(conf)) { agent ⇒
      def atShutdown() = {
        agent.executeCommand(Terminate(sigtermProcesses = true, sigkillProcessesAfter = Some(OnJavaShutdownSigkillProcessesAfter)))
        awaitResult(agent.terminated, ShutdownTimeout)
      }
      onJavaShutdown(atShutdown, name = AgentMain.getClass.getName) {
        agent.run()
      }
    }

  private def onJavaShutdown[A](atShutdown: () ⇒ Unit, name: String)(body: ⇒ A): A = {
    val hook = new Thread {
      setName(name)
      override def run() = atShutdown()
    }
    Runtime.getRuntime.addShutdownHook(hook)
    try body
    finally Runtime.getRuntime.removeShutdownHook(hook)
  }
}
