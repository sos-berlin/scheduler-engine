package com.sos.scheduler.engine.plugins.jetty.log

import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests
import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests.javaResource
import com.sos.scheduler.engine.test.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.WebResource
import java.io.{BufferedReader, Reader}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.slf4j.LoggerFactory

//TODO Wechsel der Datei bei Log.start_new_file() und instance_number berücksichtigen
//TODO Datei selbst löschen, wenn Servlet länger lebt als Prefix_log?

@RunWith(classOf[JUnitRunner])
final class LogServletIT extends ScalaSchedulerTest {

  import LogServletIT._

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(JettyPluginTests.getClass.getPackage))

  private lazy val resource = javaResource(injector)

  ignore("Read a task log") {
    startLogThread(resource.path("job.log").queryParam("job", "a"))
    for (i <- 1 to 3) {
      Thread.sleep(1000)
      scheduler.executeXml(<start_job job='/a'/>)
    }
    Thread.sleep(100)
  }

  // Fehler SCHEDULER-291  Error when removing protocol file: ERRNO-13  Permission denied
  ignore("Read an order log") {
    scheduler.executeXml(<order job_chain='/a' id='1'/>)
    startLogThread(resource.path("order.log").queryParam("job_chain", "a").queryParam("order", "1"))
    Thread.sleep(1000)
  }

  private def startLogThread(r: WebResource) {
    new Thread("LogReader " + r) {
      override def run() {
        logReader(r.accept(TEXT_PLAIN_TYPE).get(classOf[Reader]))
        logger.info("---------------------")
      }
    }.start()
  }
}

private object LogServletIT {
  val logger = LoggerFactory.getLogger(classOf[LogServletIT])

  def logReader(reader: Reader) {
    val r = new BufferedReader(reader)
    while (true) {
      val line = r.readLine()
      if (line == null) return
      logger.info(line)
    }
  }
}
