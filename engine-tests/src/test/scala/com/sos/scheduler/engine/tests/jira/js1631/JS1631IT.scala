package com.sos.scheduler.engine.tests.jira.js1631

import com.sos.scheduler.engine.agent.test.AgentConfigDirectoryProvider.{PrivateHttpJksResource, PublicHttpJksResource}
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1631IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected def newAgentConfiguration() = {
    PrivateHttpJksResource.copyToFile(testEnvironment.directory / "agent/config/private/private-https.jks")
    super.newAgentConfiguration().copy(
      dataDirectory = Some(testEnvironment.agentDirectory),
      httpPort = None)
      .withHttpsPort(findRandomFreeTcpPort())
  }

  "Run job over HTTPS Agent" in {
    PublicHttpJksResource.copyToFile(testEnvironment.configDirectory / "agent-https.jks")
    runJob(JobPath("/test"))
  }
}
