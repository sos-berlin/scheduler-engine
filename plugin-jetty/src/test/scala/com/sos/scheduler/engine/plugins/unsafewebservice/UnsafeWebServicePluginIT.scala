package com.sos.scheduler.engine.plugins.unsafewebservice

import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester.normalizeUri
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class UnsafeWebServicePluginIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  "Execute a show command via GET" in {
    getCommand("<show_state/>") should include ("<state")
  }

  "Execute a show command without XML syntax via GET" in {
    getCommand("show_state") should include ("<state")
    getCommand("s") should include ("<state")
  }

  "Execute a modifying command via GET" in {
    getCommand("<check_folders/>") should include ("<ok")
  }

  def getCommand(command: String) =
    get[String](normalizeUri(s"/jobscheduler/engine/UNSAFE/command?command=$command"), Accept = List(TEXT_XML_TYPE))
}
