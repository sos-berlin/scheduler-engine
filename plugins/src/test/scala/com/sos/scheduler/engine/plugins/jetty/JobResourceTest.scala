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
final class JobResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import JobResourceTest._

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val jobResource = newAuthResource(contextUri+"/objects/job").queryParam("job", "a")

  test("Read a job snapshot log") {
    val log = jobResource.path("log.snapshot").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    log should include ("SCHEDULER-893")
  }
}

object JobResourceTest {
  private val logger = Logger.getLogger(classOf[JobResourceTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPlugin.prefixPath)
}
