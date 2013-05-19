package com.sos.scheduler.engine.plugins.jetty.cpp

import CppServletIT._
import com.google.common.io.Files
import com.google.inject.Injector
import com.sos.scheduler.engine.data.job.{TaskStartedEvent, TaskEndedEvent}
import com.sos.scheduler.engine.kernel.settings.SettingName
import com.sos.scheduler.engine.plugins.jetty.Config._
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests._
import com.sos.scheduler.engine.test.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.{Client, ClientResponse, UniformInterfaceException}
import java.io.File
import java.net.URI
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests

@RunWith(classOf[JUnitRunner])
final class CppServletIT extends ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(JettyPluginTests.getClass.getPackage))

  private val httpDirectory = controller.environment.directory

  override protected def checkedBeforeAll() {
    controller.getSettings.set(SettingName.htmlDir, httpDirectory.getPath)    // Für Bitmuster-Test
    controller.activateScheduler()
  }

  for (testConf <- TestConf(newAuthentifyingClient(), withGzip = false) ::
                   //TestConf(newAuthentifyingClient(filters=Iterable(new GZIPContentEncodingFilter(false))), withGzip = true) ::
                   Nil) {
    lazy val resource = cppResource(injector, testConf.client)

    test("Kommando über POST "+testConf) {
      val result = stringFromResponse(resource.`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[ClientResponse], "<show_state/>"))
      result should include ("<state")
    }

    test("Kommando über POST ohne Authentifizierung "+testConf) {
      val webClient = Client.create()
      val x = intercept[UniformInterfaceException] {
        cppResource(injector, webClient).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
      }
      webClient.destroy()
      x.getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
    }

    test("Kommando über GET "+testConf) {
      val result = stringFromResponse(resource.path("<show_state/>").accept(TEXT_XML_TYPE).get(classOf[ClientResponse]))
      result should include ("<state")
    }

    test("Alle Bitmuster "+testConf) {
      val bytes = (0 to 255 map { _.toByte }).toArray
      val filename = "test.txt"
      Files.write(bytes, new File(httpDirectory, filename))
      val response = checkedResponse(resource.path(filename).get(classOf[ClientResponse]))
      response.getEntity(classOf[Array[Byte]]) should equal(bytes)
    }

    test("show_log?task=... "+testConf) {
      val eventPipe = controller.newEventPipe()
      scheduler.executeXml(<order job_chain={orderKey.jobChainPathString} id={orderKey.getId.string}/>)
      val taskId = eventPipe.nextWithCondition { e: TaskStartedEvent => e.jobPath == orderJobPath } .taskId
      val result = stringFromResponse(resource.path("show_log").queryParam("task", taskId.string).accept(TEXT_HTML_TYPE).get(classOf[ClientResponse]))
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
  val orderKey = aJobChainPath.orderKey("1")

  def cppResource(injector: Injector, client: Client) =
    client.resource(cppContextUri(injector))

  def cppContextUri(injector: Injector) =
    new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + cppPrefixPath)

  case class TestConf(client: Client, withGzip: Boolean) {
    override def toString = if (withGzip) "compressed with gzip" else ""
  }
}
