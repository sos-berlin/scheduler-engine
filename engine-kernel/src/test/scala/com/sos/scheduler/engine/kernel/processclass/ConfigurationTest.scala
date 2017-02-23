package com.sos.scheduler.engine.kernel.processclass

import com.sos.jobscheduler.common.scalautil.xmls.XmlSources.xmlElemToSource
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.http.client.heartbeat.HttpHeartbeatTiming
import com.sos.scheduler.engine.kernel.processclass.agent.Agent
import com.sos.scheduler.engine.kernel.processclass.common.selection.{FixedPriority, RoundRobin}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ConfigurationTest extends FreeSpec {

  "XML parser" - {
    "empty" in {
      check(<process_class/>, Configuration())
    }

    "Legacy remote_scheduler" in {
      check(
        <process_class remote_scheduler="A"/>,
        Configuration(Some(Agent(0, AgentAddress("A"), None)))
      )
    }

    "Agent bundle" in {
      check(
        <process_class>
          <remote_schedulers>
            <remote_scheduler remote_scheduler="A"/>
            <remote_scheduler remote_scheduler="B"/>
          </remote_schedulers>
        </process_class>,
        Configuration(
          None,
          Vector(Agent(0, AgentAddress("A"), None), Agent(1, AgentAddress("B"), None)),
          FixedPriority)
      )
    }

    "Agent bundle with legacy attribute, select=next" in {
      check(
        <process_class remote_scheduler="LEGACY">
          <remote_schedulers select="next">
            <remote_scheduler remote_scheduler="A"/>
            <remote_scheduler remote_scheduler="B"/>
          </remote_schedulers>
        </process_class>,
        Configuration(
          Some(Agent(0, AgentAddress("LEGACY"), None)),
          Vector(Agent(1, AgentAddress("A"), None), Agent(2, AgentAddress("B"), None)),
          RoundRobin)
      )
    }

    "Agent bundle, select=first" in {
      check(
        <process_class>
          <remote_schedulers select="first">
            <remote_scheduler remote_scheduler="A" http_heartbeat_timeout="100"/>
            <remote_scheduler remote_scheduler="B" http_heartbeat_timeout="100" http_heartbeat_period="10"/>
          </remote_schedulers>
        </process_class>,
        Configuration(None,
          Vector(
            Agent(0, AgentAddress("A"), Some(HttpHeartbeatTiming(timeout = 100.s, period = 50.s))),
            Agent(1, AgentAddress("B"), Some(HttpHeartbeatTiming(timeout = 100.s, period = 10.s)))),
          FixedPriority)
      )
    }

    def check(elem: xml.Elem, configuration: Configuration) = {
      assert(Configuration.parse(elem) == configuration)
    }
  }
}
