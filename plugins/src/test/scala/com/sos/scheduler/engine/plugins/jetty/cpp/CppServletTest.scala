package com.sos.scheduler.engine.plugins.jetty.cpp

import com.google.common.io.Files
import com.google.inject.Injector
import com.sos.scheduler.engine.kernel.settings.SettingName
import com.sos.scheduler.engine.plugins.jetty.Config._
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.{Client, ClientResponse}
import java.io.File
import java.net.URI
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
final class CppServletTest extends ScalaSchedulerTest {
  import CppServletTest._

  override val configurationPackage = classOf[JettyPlugin].getPackage
  private val httpDirectory = controller.environment.directory

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.getSettings.set(SettingName.htmlDir, httpDirectory.getPath)    // Für Bitmuster-Test
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
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
      val x = intercept[com.sun.jersey.api.client.UniformInterfaceException] {
        cppResource(injector, Client.create()).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
      }
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
      scheduler.executeXml(<order job_chain={jobChainPath} id={orderId}/>)
      Thread.sleep(500)  //TODO TaskStartedEvent
      val result = stringFromResponse(resource.path("show_log").queryParam("task", "1").accept(TEXT_HTML_TYPE).get(classOf[ClientResponse]))
      result should include ("SCHEDULER-918  state=closed")
      result should include ("SCHEDULER-962") // "Protocol ends in ..."
      result.trim should endWith("</html>")
    }

    def stringFromResponse(r: ClientResponse) = checkedResponse(r).getEntity(classOf[String])

    def checkedResponse(r: ClientResponse) = {
      assert(fromStatusCode(r.getStatus) === OK, "Unexpected HTTP status "+r.getStatus)
      if (testConf.withGzip) assert(r.getEntityInputStream.isInstanceOf[GZIPInputStream], r.getEntityInputStream.getClass +" should be a GZIPInputStream")
      else assert(!r.getEntityInputStream.isInstanceOf[GZIPInputStream], r.getEntityInputStream.getClass +" should not be a GZIPInputStream")
      r
    }
  }
}

object CppServletTest {
  //private val logger = Logger.getLogger(classOf[CppServletTest])
  //Logger.getLogger(classOf[CppServlet]).setLevel(Level.ALL)

  private val jobChainPath = "a"
  private val orderId = "1"

  def cppResource(injector: Injector, client: Client) = client.resource(cppContextUri(injector))

  private def cppContextUri(injector: Injector) = new URI("http://localhost:"+ jettyPortNumber(injector) + contextPath + cppPrefixPath)

  private case class TestConf(client: Client, withGzip: Boolean) {
    override def toString = if (withGzip) "compressed with gzip" else ""
  }
}
