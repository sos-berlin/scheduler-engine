package com.sos.scheduler.engine.plugins.jetty.cpp

import com.google.common.io.Files
import com.sos.scheduler.engine.data.job.TaskStarted
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.plugins.jetty.cpp.CppServletIT._
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import com.sos.scheduler.engine.plugins.jetty.test.{JettyPluginJerseyTester, JettyPluginTests}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse.Status._
import com.sun.jersey.api.client.{Client, ClientResponse, UniformInterfaceException}
import java.io.File
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class CppServletIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  private lazy val httpDirectory = testDirectory

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(JettyPluginTests.getClass.getPackage),
    cppSettings = CppSettings.TestMap + (CppSettingName.htmlDir -> httpDirectory.getPath))    // Für Bitmuster-Test

  "Kommando ueber POST" in {
    val result = stringFromResponse(webResource.path("/jobscheduler/engine-cpp").`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[ClientResponse], "<show_state/>"))
    result should include ("<state")
  }

  "Kommando ueber POST ohne Authentifizierung" in {
    val webClient = Client.create()
    intercept[UniformInterfaceException] {
      webClient.resource(s"http://$host:$port/jobscheduler/engine-cpp").`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
    } .getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
    webClient.destroy()
  }

  "Kommando ueber GET" in {
    val result = stringFromResponse(get[ClientResponse]("/jobscheduler/engine-cpp/%3Cshow_state/%3E", Accept = List(TEXT_XML_TYPE)))
    result should include ("<state")
  }

  "Forbidden modifying command via GET" in {
    get[ClientResponse]("/jobscheduler/engine-cpp/%3Cupdate_folders/%3E", Accept = List(TEXT_XML_TYPE))
      .getStatus shouldEqual FORBIDDEN.getStatusCode
  }

  "Alle Bitmuster" in {
    val bytes = (0 to 255 map { _.toByte }).toArray
    val filename = "test.txt"
    Files.write(bytes, new File(httpDirectory, filename))
    val response = checkedResponse(get[ClientResponse](s"/jobscheduler/engine-cpp/$filename"))
    response.getEntity(classOf[Array[Byte]]) should equal(bytes)
  }

  "show_log?task=..." in {
    val eventPipe = controller.newEventPipe()
    scheduler.executeXml(<order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}/>)
    val taskId = eventPipe.nextWithCondition[TaskStarted] { _.jobPath == OrderJobPath } .taskId
    val result = stringFromResponse(get[ClientResponse](s"/jobscheduler/engine-cpp/show_log?task=${taskId.string}", Accept = List(TEXT_HTML_TYPE)))
    result should include ("SCHEDULER-918  state=closed")
    result should include ("SCHEDULER-962") // "Protocol ends in ..."
    result.trim should endWith("</html>")
  }

  def stringFromResponse(r: ClientResponse) =
    checkedResponse(r).getEntity(classOf[String])

  def checkedResponse(r: ClientResponse) = {
    assert(fromStatusCode(r.getStatus) === OK, "Unexpected HTTP status "+r.getStatus)
    val isZipped = r.getEntityInputStream.isInstanceOf[GZIPInputStream]
    //if (testConf.withGzip) assert(isZipped, r.getEntityInputStream.getClass +" should be a GZIPInputStream") else
    assert(!isZipped, r.getEntityInputStream.getClass +" should not be a GZIPInputStream")
    r
  }
}

private object CppServletIT {
  private val orderKey = AJobChainPath.orderKey("1")
}
