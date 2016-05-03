package com.sos.scheduler.engine.test.agent

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.client.{AgentClient, AgentClientFactory}
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent
import com.sos.scheduler.engine.eventbus.EventSubscription
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest

/**
 * @author Joacim Zschimmer
 */
trait AgentWithSchedulerTest extends HasCloser {
  this: ScalaSchedulerTest ⇒

  protected final lazy val agent = {
    val agent = new Agent(newAgentConfiguration())
    eventBus register EventSubscription[SchedulerCloseEvent] { _ ⇒ agent.close() }   // Shutdown the server Agent after the client Engine
    // Never unregistered, Scheduler closes anyway
    agent
  }
  protected final lazy val agentUri = agent.localUri
  protected final lazy val agentClient: AgentClient = instance[AgentClientFactory].apply(agent.localUri)

  protected def newAgentConfiguration() = AgentConfiguration.forTest()

  protected override def onSchedulerActivated() = {
    val started = agent.start()
    scheduler executeXml <process_class name={AgentProcessClassPath.withoutStartingSlash} remote_scheduler={agentUri}/>
    awaitResult(started, 10.s)
  }
}

object AgentWithSchedulerTest {
  val AgentProcessClassPath = ProcessClassPath("/test-agent")

  implicit class AgentJobPath(val s: JobPath) extends AnyVal {
    def asAgent = JobPath(s.string.concat("-agent"))
  }
}
