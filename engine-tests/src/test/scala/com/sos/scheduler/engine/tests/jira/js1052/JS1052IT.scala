package com.sos.scheduler.engine.tests.jira.js1052

import JS1052IT._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.{findRandomFreeTcpPort, alternateTcpPortRange}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.io.{OutputStreamWriter, InputStreamReader}
import java.net.Socket
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/** JS-1052
 * @author Florian
 */
@RunWith(classOf[JUnitRunner])
final class JS1052IT extends FunSuite with ScalaSchedulerTest {
  private lazy val supervisorPortNumber = findRandomFreeTcpPort(alternateTcpPortRange)
  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$supervisorPortNumber"))

  test("JS-1052") {
    val sockets = mutable.Buffer[Socket]()

    for (i <- 1 to maxConnections) {   // Unter dem Maximum 1024 von Unix
      val socket = new Socket("127.0.0.1", supervisorPortNumber)
      sockets += socket
      register(socket)
    }

    def register(socket: Socket): Unit = {
      val writer = new OutputStreamWriter(socket.getOutputStream)
      val reader = new InputStreamReader(socket.getInputStream)
      writer.write("<register_remote_scheduler scheduler_id='TEST-SCHEDULER-ID' tcp_port='4444' version='1.5.3251'/>")
      writer.flush()
      reader.read() should equal ('<'.toInt)
    }
  }
}

private object JS1052IT {
  private val maxConnections = if (isWindows) 2000 else 950/2   // Unter Unix unterm Maximum von 1024, aufgeteilt zwischen Test und Scheduler (im selben Prozess)
}
