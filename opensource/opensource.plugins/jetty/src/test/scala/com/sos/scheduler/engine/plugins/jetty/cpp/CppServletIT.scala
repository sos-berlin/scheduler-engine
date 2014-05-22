package com.sos.scheduler.engine.plugins.jetty.cpp

import CppServletIT._
import com.google.common.io.Files
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.kernel.settings.{CppSettings, CppSettingName}
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import com.sos.scheduler.engine.plugins.jetty.test.{JettyPluginJerseyTester, JettyPluginTests}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.{Client, ClientResponse, UniformInterfaceException}
import java.io.File
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status._
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
    cppSettings = CppSettings.testMap + (CppSettingName.htmlDir -> httpDirectory.getPath))    // Für Bitmuster-Test

  for (testConf <- TestConf(newAuthentifyingClient(), withGzip = false) ::
                   //TestConf(newAuthentifyingClient(filters=Iterable(new GZIPContentEncodingFilter(false))), withGzip = true) ::
                   Nil) {
    //lazy val resource = cppResource(injector, testConf.client)

    s"Kommando ueber POST $testConf" in {
      val result = stringFromResponse(webResource.path("/jobscheduler/engine-cpp").`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[ClientResponse], "<show_state/>"))
      result should include ("<state")
    }

    s"Kommando ueber POST ohne Authentifizierung $testConf" in {
      val webClient = Client.create()
      intercept[UniformInterfaceException] {
        webClient.resource(s"http://$host:$port/jobscheduler/engine-cpp").`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
      } .getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
      webClient.destroy()
    }

    s"Kommando ueber GET $testConf" in {
      val result = stringFromResponse(get[ClientResponse]("/jobscheduler/engine-cpp/%3Cshow_state/%3E", Accept = List(TEXT_XML_TYPE)))
      result should include ("<state")
    }

    s"Alle Bitmuster $testConf" in {
      val bytes = (0 to 255 map { _.toByte }).toArray
      val filename = "test.txt"
      Files.write(bytes, new File(httpDirectory, filename))
      val response = checkedResponse(get[ClientResponse](s"/jobscheduler/engine-cpp/$filename"))
      response.getEntity(classOf[Array[Byte]]) should equal(bytes)
    }

    s"show_log?task=... $testConf" in {
      val eventPipe = controller.newEventPipe()
      scheduler.executeXml(<order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}/>)
      val taskId = eventPipe.nextWithCondition { e: TaskStartedEvent => e.jobPath == orderJobPath } .taskId
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
      if (testConf.withGzip) assert(isZipped, r.getEntityInputStream.getClass +" should be a GZIPInputStream")
      else assert(!isZipped, r.getEntityInputStream.getClass +" should not be a GZIPInputStream")
      r
    }
  }
}

private object CppServletIT {
  private val orderKey = aJobChainPath.orderKey("1")

  private case class TestConf(client: Client, withGzip: Boolean) {
    override def toString = if (withGzip) "compressed with gzip" else ""
  }
}
