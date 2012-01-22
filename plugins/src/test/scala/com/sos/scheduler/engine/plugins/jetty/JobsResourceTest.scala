package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

import JettyPluginTest._

@RunWith(classOf[JUnitRunner])
final class JobsResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import JobsResourceTest._

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val jobsResource = newAuthResource(contextUri+"/objects/jobs")

  test("Read job list") {
    val xml = jobsResource.accept(TEXT_XML_TYPE).get(classOf[String])
    logger.info(xml)
    xml should include ("<job name=\"a\"")
  }
}

object JobsResourceTest {
  private val logger = Logger.getLogger(classOf[JobsResourceTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPluginConfiguration.prefixPath)
}
