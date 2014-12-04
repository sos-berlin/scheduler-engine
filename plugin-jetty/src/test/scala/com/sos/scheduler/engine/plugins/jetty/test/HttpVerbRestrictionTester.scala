package com.sos.scheduler.engine.plugins.jetty.test

import com.google.common.base.Splitter
import com.sos.scheduler.engine.plugins.jetty.test.HttpVerbRestrictionTester._
import com.sun.jersey.api.client.ClientResponse.Status.{FORBIDDEN, METHOD_NOT_ALLOWED, OK}
import com.sun.jersey.api.client.{ClientResponse, WebResource}
import org.scalatest.Matchers._
import scala.collection.JavaConversions._

/**
 * @author Joacim Zschimmer
 */
final class HttpVerbRestrictionTester(webResource: WebResource) {

  def checkPathForVerbs(path: String, verbSet: Set[String]): Unit = {
    httpTraceShouldBeNotAllowed(path)
    httpOptionShouldReturnVerbs(path, verbSet)
  }

  def httpTraceShouldBeNotAllowed(path: String): Unit =
    withClue("HTTP verb TRACE: ") {
      webResource.path(path).method("TRACE", classOf[ClientResponse]).getStatus should
        (equal (METHOD_NOT_ALLOWED.getStatusCode) or equal (FORBIDDEN.getStatusCode))
    }

  def httpOptionShouldReturnVerbs(path: String, verbSet: Set[String]): Unit = {
    val response = webResource.path(path).options(classOf[ClientResponse])
    if (verbSet.isEmpty) {
      response.getStatus shouldEqual METHOD_NOT_ALLOWED.getStatusCode
    } else {
      response.getStatus shouldEqual OK.getStatusCode
      val allowStrings = webResource.path(path).options(classOf[ClientResponse]).getHeaders.get("Allow")
      allowStrings.size shouldEqual 1
      (AllowSplitter split allowStrings(0)).toSet shouldEqual verbSet
    }
  }
}

object HttpVerbRestrictionTester {
  /** Methods as of [[org.eclipse.jetty.servlet.DefaultServlet]]#doOptions() implementing POST as GET. */
  val GetServletMethods = Set("OPTIONS", "GET", "HEAD")
  val DefaultServletMethods = Set("OPTIONS", "GET", "HEAD", "POST")

  private val AllowSplitter = (Splitter on "[\r\n \t]*,[\r\n \t]*".r.pattern).trimResults.omitEmptyStrings
}
