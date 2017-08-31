package com.sos.scheduler.engine.tests.jira.js1721

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.test.SharedDatabaseTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import JS1721IT._
import scala.util.control.NonFatal

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1721IT extends FreeSpec with ScalaSchedulerTest with SharedDatabaseTest with JettyPluginJerseyTester {

  protected lazy val databaseTcpPort :: tcpPort :: Nil = findRandomFreeTcpPorts(2)
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = s"-tcp-port=$tcpPort" :: s"-udp-port=$tcpPort" :: "-validate-xml-" :: "-distributed-orders" :: Nil,
    database = Some(databaseConfiguration))

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
      val result = get[String]("/jobscheduler/engine/jobs")
      logger.info(s"Web services has returned $result")
    } catch { case NonFatal(t) ⇒
      logger.warn(s"Web service: $t")
    }
  }
}

private object JS1721IT {
  private val logger = Logger(getClass)
}
