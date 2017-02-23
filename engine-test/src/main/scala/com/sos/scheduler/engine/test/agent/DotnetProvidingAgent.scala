package com.sos.scheduler.engine.test.agent

import com.sos.jobscheduler.common.system.FileUtils._
import com.sos.jobscheduler.common.system.OperatingSystem._
import com.sos.jobscheduler.taskserver.dotnet.DotnetEnvironment
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest

/**
  * @author Joacim Zschimmer
  */
trait DotnetProvidingAgent extends AgentWithSchedulerTest {
  this: ScalaSchedulerTest ⇒

  override protected def newAgentConfiguration() = {
    val conf = super.newAgentConfiguration()
    if (isWindows) {
      val dotnetEnv = new DotnetEnvironment(temporaryDirectory)  // .closeWithCloser  The DLLs cannot be removed. They are still loaded.
      conf withDotnetAdapterDirectory Some(dotnetEnv.directory)
    }
    else conf
  }
}
