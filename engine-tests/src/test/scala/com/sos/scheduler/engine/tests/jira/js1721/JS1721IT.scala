package com.sos.scheduler.engine.tests.jira.js1721

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.test.SharedDatabaseTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1721.JS1721IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.util.control.NonFatal

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1721IT extends FreeSpec with ScalaSchedulerTest with SharedDatabaseTest {

  protected lazy val databaseTcpPort :: tcpPort :: httpPort :: Nil = findRandomFreeTcpPorts(3)
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = s"-tcp-port=$tcpPort" :: s"-udp-port=$tcpPort" :: s"-http-port=$httpPort" :: "-validate-xml-" :: "-distributed-orders" :: Nil,
    database = Some(databaseConfiguration))
  private lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser


  if (sys.props contains "JS-1721")  // Test exits the JVM after two minutes, so it as to be enabled explicitly
    "Cluster member restarts when database hangs for more than two minutes (exits this JVM!)" in {
      callWebService()
      @volatile var stopThread = false
      val thread = new Thread {
        override def run() = while (!stopThread) {
          callWebService()
          sleep(10.s)
        }
      }
      thread.start()
      scheduler.executeXml(<test_delay delay="30"/>)  // After 15s, ThreadLock logs an warning
      scheduler.executeXml(<test_delay delay="120"/>)
      stopThread = true
      thread.join()
      callWebService()
    }

  private def callWebService(): Unit = {
    logger.info("Calling web service ...")
    try {
      val result = client.overview await 99.s
      logger.info(s"Web services has returned $result")
    } catch { case NonFatal(t) ⇒
      logger.warn(s"Web service: $t")
    }
  }
}

private object JS1721IT {
  private val logger = Logger(getClass)
}
