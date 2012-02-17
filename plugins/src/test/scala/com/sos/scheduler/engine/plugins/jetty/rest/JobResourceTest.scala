package com.sos.scheduler.engine.plugins.jetty.rest

import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.util.JettyPluginTests.{javaContextUri, newAuthResource}
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class JobResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  override val configurationPackage = classOf[JettyPlugin].getPackage
  private lazy val jobResource = newAuthResource(javaContextUri(injector) + "/job").queryParam("job", "a")

  test("Read a job configuration") {
    val result = jobResource.path("configuration").accept(TEXT_XML_TYPE).get(classOf[String])
    result should include ("<job")
  }

  test("Read a job description") {
    val result = jobResource.path("description").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    result should equal ("TEST-DESCRIPTION mit Ümläüten")
  }

  test("Read a job snapshot log") {
    val log = jobResource.path("log.snapshot").accept(TEXT_PLAIN_TYPE).get(classOf[String])
    log should include ("SCHEDULER-893")
  }
}

//object JobResourceTest {
//  private val logger = Logger.getLogger(classOf[JobResourceTest])
//}
