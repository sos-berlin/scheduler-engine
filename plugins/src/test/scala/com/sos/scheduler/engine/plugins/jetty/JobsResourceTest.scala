package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import JettyPluginTests._

@RunWith(classOf[JUnitRunner])
final class JobsResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  private lazy val jobsResource = newAuthResource(javaContextUri(injector)+"/objects/jobs")

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  test("Read job list") {
    val xml = jobsResource.accept(TEXT_XML_TYPE).get(classOf[String])
    xml should include ("<job name=\"a\"")
    //assertXpathEvaluatesTo(<job name='a'/>.toString(), "/scheduler/job[@name='a']", xml)
  }
}
