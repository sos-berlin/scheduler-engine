package com.sos.scheduler.engine.agent.test

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.test.AgentTest._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
trait AgentTest extends HasCloser {
  this: ScalaSchedulerTest ⇒

  private lazy val agentTcpPort = findRandomFreeTcpPort()
  private lazy val agent = new Agent(AgentConfiguration(httpPort = agentTcpPort, httpInterfaceRestriction = Some("127.0.0.1"))).closeWithCloser

  protected override def onSchedulerActivated() = {
    val started = agent.start()
    scheduler executeXml <process_class name={AgentProcessClassPath.withoutStartingSlash} remote_scheduler={s"http://127.0.0.1:$agentTcpPort"}/>
    awaitResult(started, 10.seconds)
  }
}

object AgentTest {
  val AgentProcessClassPath = ProcessClassPath("/test-agent")
}
