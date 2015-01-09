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
  }

  "CloseRemoteTask" in {
    intercept[Exception] { parse(<remote_scheduler.remote_task.close/>) }
    parse(<remote_scheduler.remote_task.close process_id="1112223334445556667"/>) shouldEqual
      CloseRemoteTask(RemoteTaskId(1112223334445556667L), kill = false)
    parse(<remote_scheduler.remote_task.close process_id="1112223334445556667" kill="true"/>) shouldEqual
      CloseRemoteTask(RemoteTaskId(1112223334445556667L), kill = true)
  }

  private def parse(elem: xml.Elem) = Command.parseString(elem.toString())
}
