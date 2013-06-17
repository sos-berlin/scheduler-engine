package com.sos.scheduler.engine.plugins.jetty.services

import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests.javaResource
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class JobsServiceIT extends ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(classOf[JettyPlugin].getPackage))

  private lazy val jobsResource = javaResource(injector).path("jobs")

  test("Read job list") {
    val xml = jobsResource.accept(TEXT_XML_TYPE).get(classOf[String])
    xml should include ("<job name=\"a\"")
    //assertXpathEvaluatesTo(<job name='a'/>.toString(), "/scheduler/job[@name='a']", xml)
  }
}
