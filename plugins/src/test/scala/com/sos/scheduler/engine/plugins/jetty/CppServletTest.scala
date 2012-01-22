package com.sos.scheduler.engine.plugins.jetty

import java.net.URI
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status.UNAUTHORIZED
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

import JettyPluginTest._
import com.sun.jersey.api.client.Client
import org.junit.Ignore

//@RunWith(classOf[JUnitRunner])
@Ignore
final class CppServletTest extends ScalaSchedulerTest {
  import CppServletTest._

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val authResource = newAuthentifyingResource(contextUri)
  private val nonAuthResource = Client.create().resource(contextUri)

  test("Kommando über POST") {
    val result = authResource.`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
    result should include ("<state")
  }

  //TODO Alle Tests mit und ohne Authentifizierung. Generisch codieren.
  test("Kommando über POST ohne Authentifizierung") {
    val x = intercept[com.sun.jersey.api.client.UniformInterfaceException] {
      nonAuthResource.`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
    }
    x.getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
  }

  test("Kommando über GET") {
    val result = authResource.path("<show_state/>").accept(TEXT_XML_TYPE).get(classOf[String])
    result should include ("<state")
  }

  test("show_log?task=...") {
    scheduler.executeXml(<order job_chain={jobChainPath} id={orderId}/>)
    Thread.sleep(500)  //TODO TaskStartedEvent
    // Wir nehmen ein Task-Protokoll, denn ein Auftragsprotokoll führt zu SCHEDULER-291  Error when removing protocol file: ERRNO-13  Permission denied [unlink] [...\log/order.a.1.log]
    //val result = cppResource.path("show_log").queryParam("job_chain", jobChainPath).queryParam("order", orderId)
    val result = authResource.path("show_log").queryParam("task", "1").accept(TEXT_HTML_TYPE).get(classOf[String])
    //logger.info(result)
    result should include("SCHEDULER-918  state=closed")
    result should include("SCHEDULER-962")  // "Protocol ends in ..."
    result.trim should endWith("</html>")
  }
}

object CppServletTest {
  private val logger = Logger.getLogger(classOf[CppServletTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPlugin.cppPrefixPath)
  private val jobChainPath = "a"
  private val orderId = "1"
}
