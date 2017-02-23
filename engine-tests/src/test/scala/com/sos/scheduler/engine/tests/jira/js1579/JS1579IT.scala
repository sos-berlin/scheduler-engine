package com.sos.scheduler.engine.tests.jira.js1579

import akka.util.ByteString
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1579.JS1579IT._
import java.net.InetSocketAddress
import java.nio.channels.SocketChannel
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1579 In XML disallowed characters are suppressed.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1579IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))

  "In XML attributes set by C++, disallowed XML characters are suppressed" in {
    scheduler executeXml <show_state/>
    val channel = SocketChannel.open(new InetSocketAddress("127.0.0.1", tcpPort))
    val badCommand = s"<BAD-XML>${(InvalidXmlCharacters - '\u0000').mkString}</WORSE-XML..."
    channel.write(ByteString(badCommand).toByteBuffer)
    sleep(100.ms)
    val a = (scheduler.executeXml(<show_state/>).answer \ "state" \ "connections" \ "connection" \ "operation" \ "xml_operation" ).toString
    assert(a contains "&lt;BAD-XML&gt;����������������������������&lt;/WORSE-XML...")
    channel.close()
  }

  "In XML text containing stdout job output, disallowed XML characters are suppressed" in {
    val jobPaths = List(JobPath("/test"), JobPath("/test-agent"))
    (for (j ← jobPaths) yield
      for (result ← startJob(j).result) yield
        assert(result.logString contains "INVALID-->�����������������������������<--INVALID")
    ) await TestTimeout
  }
}

private[js1579] object JS1579IT {
  val InvalidXmlCharacters = Set(
    '\u0000', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005', '\u0006', '\u0007',
    '\u0008',                     '\u000b', '\u000c',           '\u000e', '\u000f',
    '\u0010', '\u0011', '\u0012', '\u0013', '\u0014', '\u0015', '\u0016', '\u0017',
    '\u0018', '\u0019', '\u001a', '\u001b', '\u001c', '\u001d', '\u001e', '\u001f')
}
