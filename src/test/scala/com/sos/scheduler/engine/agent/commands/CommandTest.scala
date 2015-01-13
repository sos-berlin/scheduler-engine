package com.sos.scheduler.engine.agent.commands

import com.sos.scheduler.engine.data.agent.RemoteTaskId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class CommandTest extends FreeSpec {
  "StartRemoteTask" in {
    intercept[Exception] { parse(<remote_scheduler.start_remote_task/>) }
    parse(<remote_scheduler.start_remote_task tcp_port="999"/>) shouldEqual
      StartRemoteTask(controllerTcpPort = 999, usesApi = true, javaOptions = "", javaClassPath = "")
    parse(<remote_scheduler.start_remote_task tcp_port="999" kind="process" java_options="OPTIONS" java_classpath="CLASSPATH"/>) shouldEqual
      StartRemoteTask(controllerTcpPort = 999, usesApi = false, javaOptions = "OPTIONS", javaClassPath = "CLASSPATH")
    intercept[Exception] { parse(<remote_scheduler.start_remote_task tcp_port="-1"/>) }
    intercept[Exception] { parse(<remote_scheduler.start_remote_task tcp_port="0"/>) }
    intercept[Exception] { parse(<remote_scheduler.start_remote_task tcp_port="65536"/>) }
    parse(<remote_scheduler.start_remote_task tcp_port="65535"/>)
  }

  "CloseRemoteTask" in {
    intercept[Exception] { parse(<remote_scheduler.remote_task.close/>) }
    parse(<remote_scheduler.remote_task.close process_id="111222333444555666"/>) shouldEqual
      CloseRemoteTask(RemoteTaskId(111222333444555666L), kill = false)
    parse(<remote_scheduler.remote_task.close process_id="111222333444555666" kill="true"/>) shouldEqual
      CloseRemoteTask(RemoteTaskId(111222333444555666L), kill = true)
  }

  private def parse(elem: xml.Elem) = Command.parseString(elem.toString())
}
