package com.sos.scheduler.engine.plugins.newwebservice

import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.test.json.JsonRegexMatcher._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse.Status._
import com.sun.jersey.api.client.{ClientResponse, UniformInterfaceException}
import java.net.URI
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class NewWebServicePluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  "overview" in {
    checkRegexJson(
      json = get[String]("/new/engine/main/overview"),
      patternMap = Map(
        "version" → """\d+\..+""".r,
        "versionCommitHash" → ".*".r,
        "startInstant" → AnyIsoTimestamp,
        "instant" → AnyIsoTimestamp,
        "schedulerId" → "test",
        "processId" → AnyInt,
        "state" → "running"))
  }

  "/" in {
    pending
    val response = get[ClientResponse]("/")
    response.getStatus shouldEqual TEMPORARY_REDIRECT.getStatusCode
    response.getLocation shouldEqual new URI("z/gui/")
  }

  "/new" in {
    pendingUntilFixed {  // Liefert Verzeichnis statt index.html
      get[String]("/new/z") should include ("<html>")
    }
  }

  "/new/" in {
    pending
    get[String]("/new/z/") should include ("<html>")
  }

  "/new/index.html" in {
    pending
    get[String]("/new/z/index.html") should include ("<html>")
  }

  "../ should be rejected" in {
    pending
    pendingUntilFixed {  // Spray scheint führende .. zu entfernen und andere zu verrechnen
      intercept[UniformInterfaceException] { get[String]("/new/z/../index.html") }
        .getResponse.getStatus shouldEqual NOT_FOUND.getStatusCode
    }
  }

  "/OutOfMemoryError" in {
    intercept[UniformInterfaceException] { get[String]("/new/OutOfMemoryError") }
      .getResponse.getStatus shouldEqual INTERNAL_SERVER_ERROR.getStatusCode
    get[String]("/new/engine/main/overview")
  }

  "/unknown" in {
    intercept[UniformInterfaceException] { get[String]("/new/engine-unknown") }
  }

  "/ERROR" in {
    intercept[UniformInterfaceException] { get[String]("/new/ERROR") }
      .getResponse.getStatus shouldEqual INTERNAL_SERVER_ERROR.getStatusCode
  }
}
