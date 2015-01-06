package com.sos.scheduler.engine.agenttest

import com.sos.scheduler.engine.agent.Main
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agenttest.AgentIT._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val agentTcpPort = findRandomFreeTcpPort()
  private lazy val agentApp = new Main(AgentConfiguration(httpPort = agentTcpPort, httpInterfaceRestriction = Some("127.0.0.1"))).closeWithCloser

  protected override def onSchedulerActivated(): Unit = {
    val started = agentApp.start()
    scheduler executeXml <process_class name="test-agent" remote_scheduler={s"http://127.0.0.1:$agentTcpPort"}/>
    Await.result(started, 5.seconds)
  }

  "Java Agent" in {
    runJobAndWaitForEnd(TestJobPath)
  }
}

private object AgentIT {
  private val TestJobPath = JobPath("/test")
}
