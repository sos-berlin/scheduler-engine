package com.sos.scheduler.engine.tests.jira.js1703

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest.AgentProcessClassPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1703.JS1703IT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1703 Timeout for unreachable Agents.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1703IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected def startAgent = false

  "Timeout for unreachable Agent" in {
    scheduler.executeXml(
      <process_class name={AgentProcessClassPath.withoutStartingSlash} timeout="2">
        <remote_schedulers>
          <remote_scheduler remote_scheduler={agentUri}/>
          <remote_scheduler remote_scheduler={agentUri}/>
        </remote_schedulers>
      </process_class>)
    val t = now
    val orderRun = startOrder(TestJobChainPath orderKey "1").result await 99.s
    assert(now - t > 2.s)
    assert(orderRun.nodeId == NodeId("ERROR"))
  }
}

private object JS1703IT {
  private val TestJobChainPath = JobChainPath("/test")
}
