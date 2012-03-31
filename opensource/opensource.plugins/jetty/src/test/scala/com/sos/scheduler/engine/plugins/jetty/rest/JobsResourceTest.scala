package com.sos.scheduler.engine.plugins.jetty.rest

import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests.javaResource
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class JobsResourceTest extends ScalaSchedulerTest {
  override val configurationPackage = classOf[JettyPlugin].getPackage
  private lazy val jobsResource = javaResource(injector).path("jobs")

  test("Read job list") {
    val xml = jobsResource.accept(TEXT_XML_TYPE).get(classOf[String])
    xml should include ("<job name=\"a\"")
    //assertXpathEvaluatesTo(<job name='a'/>.toString(), "/scheduler/job[@name='a']", xml)
  }
}
