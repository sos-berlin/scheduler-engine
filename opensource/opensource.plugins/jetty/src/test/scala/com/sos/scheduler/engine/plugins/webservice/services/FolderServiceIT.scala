package com.sos.scheduler.engine.plugins.webservice.services

import com.fasterxml.jackson.databind.ObjectMapper
import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests.javaResource
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.io.Reader
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class FolderServiceIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(Tests.testPackage))

  private lazy val resource = javaResource(injector).path("folder")
  private val objectMapper = new ObjectMapper

  test("Read job list as JSON") {
    val reader = resource.queryParam("type", "job").queryParam("folder", "/").accept(APPLICATION_JSON_TYPE).get(classOf[Reader])
    val tree = objectMapper.readValue(reader, classOf[java.util.Map[String,Object]])
    assert(tree.get("folderPath") === "/")
    assert(tree.get("typeName") === "job")
    val names = tree.get("entries").asInstanceOf[java.util.List[_]]
    assert(names.size > 0)
  }

//  ignore("Read job list as XML") {
//    val doc = resource.queryParam("type", "job").queryParam("folder", "/").accept(TEXT_XML_TYPE).get(classOf[Document])
//    assert(stringXPath(doc, "/names/name[@name='a']/@uri") != "")
//  }

//  test("Read job list as HTML") {
//    val result = resource.queryParam("type", "job").queryParam("folder", "/").accept(TEXT_HTML_TYPE).get(classOf[String])
//    assert(result contains "<html")
//    assert(result contains "/a")
//  }
}
