package com.sos.scheduler.engine.agent.test

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.test.AgentTest._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
trait AgentTest extends HasCloser {
  this: ScalaSchedulerTest ⇒

  protected lazy val agent = Agent.forTest().closeWithCloser
  protected lazy val agentUri = agent.localUri

  protected override def onSchedulerActivated() = {
    val started = agent.start()
    scheduler executeXml <process_class name={AgentProcessClassPath.withoutStartingSlash} remote_scheduler={agentUri}/>
    awaitResult(started, 10.seconds)
  }
}

object AgentTest {
  val AgentProcessClassPath = ProcessClassPath("/test-agent")
}
