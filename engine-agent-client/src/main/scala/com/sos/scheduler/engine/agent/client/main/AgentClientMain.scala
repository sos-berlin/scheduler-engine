package com.sos.scheduler.engine.agent.client.main

import com.sos.scheduler.engine.agent.client.TextAgentClient
import com.sos.scheduler.engine.common.commandline.CommandLineArguments
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Logger
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
object AgentClientMain {

  private val logger = Logger(getClass)

  def main(args: Array[String]): Unit =
    try run(args, println)
    catch { case NonFatal(t) ⇒
      println(t.toString)
      logger.error(t.toString, t)
      System.exit(1)
    }

  private[client] def run(args: Seq[String], print: String ⇒ Unit): Unit = {
    val (agentUri, operations) = parseArgs(args)
    autoClosing(new TextAgentClient(agentUri, print)) { client ⇒
      if (operations.isEmpty)
        client.requireIsResponding()
      else
        operations foreach {
          case StringCommand(command) ⇒ client.executeCommand(command)
          case StdinCommand ⇒ client.executeCommand(io.Source.stdin.mkString)
          case Get(uri) ⇒ client.get(uri)
        }
    }
  }

  private def parseArgs(args: Seq[String]) = {
    val arguments = CommandLineArguments(args)
    val agentUri = arguments.namelessValue(0) stripSuffix "/"
    val operations = arguments.namelessValues.tail map {
      case url if url startsWith "/" ⇒ Get(url)
      case "-" ⇒ StdinCommand
      case command ⇒ StringCommand(command)
    }
    if((operations count { _ == StdinCommand }) > 1) throw new IllegalArgumentException("Stdin ('-') can only be read once")
    arguments.requireNoMoreArguments()
    (agentUri, operations)
  }

  private sealed trait Operation
  private case class StringCommand(command: String) extends Operation
  private case object StdinCommand extends Operation
  private case class Get(uri: String) extends Operation
}
