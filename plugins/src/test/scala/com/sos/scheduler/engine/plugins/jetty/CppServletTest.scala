package com.sos.scheduler.engine.plugins.jetty

import com.google.common.io.Files
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.filter.ClientFilter
import com.sun.jersey.api.client.{Client, ClientResponse}
import com.sos.scheduler.engine.kernel.settings.SettingName
import com.sun.jersey.api.client.filter.GZIPContentEncodingFilter
import java.io.File
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status._
import org.joda.time.Duration
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

import JettyPluginTests._

@RunWith(classOf[JUnitRunner])
final class CppServletTest extends ScalaSchedulerTest {
  import CppServletTest._

  private val httpDirectory = controller.environment.directory

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.getSettings.set(SettingName.htmlDir, httpDirectory.getPath)
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val readTimeout = new Duration(15*1000)
  private lazy val uri = cppContextUri(injector)

  for (testConf <- TestConf(newClient(), withGzip = false) ::
                   //TestConf(newClient(new GZIPContentEncodingFilter(false)), withGzip = true) ::
                   Nil) {
    val client = testConf.client

    test("Kommando über POST "+testConf) {
      val result = stringFromResponse(client.resource(uri).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[ClientResponse], "<show_state/>"))
      result should include ("<state")
    }

    test("Kommando über POST ohne Authentifizierung "+testConf) {
      val x = intercept[com.sun.jersey.api.client.UniformInterfaceException] {
        Client.create().resource(uri).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
      }
      x.getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
    }

    test("Kommando über GET "+testConf) {
      val result = stringFromResponse(client.resource(uri).path("<show_state/>").accept(TEXT_XML_TYPE).get(classOf[ClientResponse]))
      result should include ("<state")
    }

    test("Alle Bitmuster "+testConf) {
      val bytes = (0 to 255 map { _.toByte }).toArray
      val filename = "test.txt"
      Files.write(bytes, new File(httpDirectory, filename))
      val response = checkedResponse(client.resource(uri).path(filename).get(classOf[ClientResponse]))
      response.getEntity(classOf[Array[Byte]]) should equal(bytes)
    }

    test("show_log?task=... "+testConf) {
      scheduler.executeXml(<order job_chain={jobChainPath} id={orderId}/>)
      Thread.sleep(500)  //TODO TaskStartedEvent
      val result = stringFromResponse(client.resource(uri).path("show_log").queryParam("task", "1").accept(TEXT_HTML_TYPE).get(classOf[ClientResponse]))
      result should include("SCHEDULER-918  state=closed")
      result should include("SCHEDULER-962")  // "Protocol ends in ..."
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

  private def newClient(filters: ClientFilter*) = {
    val result = newAuthentifyingClient(timeout = readTimeout)
    for (f <- filters) result.addFilter(f)
    result
  }
}

object CppServletTest {
  //private val logger = Logger.getLogger(classOf[CppServletTest])
  //Logger.getLogger(classOf[CppServlet]).setLevel(Level.ALL)

  private val jobChainPath = "a"
  private val orderId = "1"

  private case class TestConf(client: Client, withGzip: Boolean) {
    override def toString = if (withGzip) "compressed with gzip" else ""
  }
}
