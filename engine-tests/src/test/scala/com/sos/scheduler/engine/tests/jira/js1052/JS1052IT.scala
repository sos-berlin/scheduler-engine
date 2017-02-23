package com.sos.scheduler.engine.tests.jira.js1052

import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.client.web.WebCommandClient
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1052.JS1052IT._
import java.io.{ByteArrayInputStream, OutputStreamWriter}
import java.net.Socket
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import org.xml.sax.InputSource
import scala.collection.mutable

/** JS-1052 Many TCP supervisor clients
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1052IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val supervisorTcpPort = findRandomFreeTcpPort()
  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(s"-tcp-port=$supervisorTcpPort"))

  s"Simulate $MaxConnections supervisor clients" in {
    def register(socket: Socket): Unit = {
      val writer = new OutputStreamWriter(socket.getOutputStream)
      writer.write(<register_remote_scheduler scheduler_id='TEST-SCHEDULER-ID' tcp_port='4444' version='1.5.3251'/>.toString())
      writer.flush()
      val b = new Array[Byte](1000)
      val length = socket.getInputStream.read(b)
      assert(length > 0)
      val len = if (b(length - 1) == '\u0000') length -1 else length
      val xmlResponseString = SafeXML.load(new InputSource(new ByteArrayInputStream(b, 0, len))).toString
      WebCommandClient.checkResponseForError(xmlResponseString)
    }

    val sockets = mutable.Buffer[Socket]()
    try
      for (i <- 1 to MaxConnections) {
        val socket = new Socket("127.0.0.1", supervisorTcpPort)
        sockets += socket
        register(socket)
      }
    finally for (o ← sockets) o.close()
  }
}

private object JS1052IT {
  private val MaxConnections = if (isWindows) 2000 else 950/2   // Unter Unix unterm Maximum von 1024, aufgeteilt zwischen Test und Scheduler (im selben Prozess)
}
