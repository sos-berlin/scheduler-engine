package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import scala.Some

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class CommandServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  "Execute a command via POST" in {
    webResource.path("/jobscheduler/engine/command").accept(TEXT_XML_TYPE).`type`(TEXT_XML_TYPE)
      .post(classOf[String], "<show_state><!--äöü--></show_state>") should include ("<state")
  }

  "Execute a show command via GET" in {
    // < %3C, > %3E
    get[String]("/jobscheduler/engine/command?command=%3Cshow_state/%3E", Accept = List(TEXT_XML_TYPE)) should include ("<state")
  }

  "Execute a show command without XML syntax via GET" in {
    get[String]("/jobscheduler/engine/command?command=show_state", Accept = List(TEXT_XML_TYPE)) should include ("<state")
  }

//  import com.sun.jersey.api.client.ClientResponse.Status.FORBIDDEN
//  import com.sun.jersey.api.client.UniformInterfaceException
//  "GET should not accept a modifying command because of HTTP 1.1 rules and CSRF" in {
//    pendingUntilFixed {  // Soll erlaubt sein.
//      intercept[UniformInterfaceException] {
//        get[String]("/jobscheduler/engine/command?command=%3Ccheck_folders/%3E", Accept = List(TEXT_XML_TYPE))
//      }
//      .getResponse.getClientResponseStatus shouldEqual FORBIDDEN
//    }
//  }
}
