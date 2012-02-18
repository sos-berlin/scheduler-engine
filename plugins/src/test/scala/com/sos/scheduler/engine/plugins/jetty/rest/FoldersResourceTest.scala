package com.sos.scheduler.engine.plugins.jetty.rest

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.kernel.util.XmlUtils._
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import org.w3c.dom.Document
import javax.ws.rs.core.MediaType._

@RunWith(classOf[JUnitRunner])
final class FoldersResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  override val configurationPackage = classOf[JettyPlugin].getPackage
  private lazy val resource = javaResource(injector).path("folders")

  test("Read job list as XML") {
    val doc = resource.queryParam("type", "job").queryParam("folder", "/").accept(TEXT_XML_TYPE).get(classOf[Document])
    assert(stringXPath(doc, "/names/name[@name='a']/@uri") != "")
  }

  test("Read job list as HTML") {
    val result = resource.queryParam("type", "job").queryParam("folder", "/").accept(TEXT_HTML_TYPE).get(classOf[String])
    assert(result contains "<html")
    assert(result contains "/a")
  }
}
