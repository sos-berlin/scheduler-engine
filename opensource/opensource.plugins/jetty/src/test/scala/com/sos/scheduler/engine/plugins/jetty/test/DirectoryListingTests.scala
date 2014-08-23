package com.sos.scheduler.engine.plugins.jetty.test

import com.sun.jersey.api.client.ClientResponse
import com.sun.jersey.api.client.ClientResponse.Status._
import org.scalatest.{Suite, TestRegistration}

/**
 * @author Joacim Zschimmer
 */
trait DirectoryListingTests {
  this: Suite with TestRegistration with JettyPluginJerseyTester ⇒

  def addDirectoryListingTests(webDirectoryName: String): Unit = {
    registerTest("Web server redirects directory url, even if access to target is forbidden") {
      val path = s"/jobscheduler/$webDirectoryName"
      withClue(s"Path $path") {
        val response = get[ClientResponse](path)
        assertResult(302)(response.getStatus) // 302 Temporary Redirect
        val redirectedUrl = response.getLocation.toString
        assertResult(webResource.path(path) + "/")(redirectedUrl)
        withClue(s"(redirected to $redirectedUrl)") {
          assertResult(FORBIDDEN) {
            get[ClientResponse](redirectedUrl).getClientResponseStatus
          }
        }
      }
    }

    registerTest("Web server do not list directory content") {
      val path = s"/jobscheduler/$webDirectoryName/"
      withClue(s"Path $path") {
        assertResult(FORBIDDEN) {
          get[ClientResponse](path).getClientResponseStatus
        }
      }
    }
  }
}
