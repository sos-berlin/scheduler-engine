package com.sos.scheduler.engine.test.agent

import com.sos.scheduler.engine.common.system.FileUtils.temporaryDirectory
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.taskserver.dotnet.DotnetEnvironment
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files.{createDirectories, createTempDirectory}

/**
  * @author Joacim Zschimmer
  */
trait DotnetProvidingAgent extends AgentWithSchedulerTest {
  this: ScalaSchedulerTest ⇒

  override protected def newAgentConfiguration() = {
    val conf = super.newAgentConfiguration()
    if (isWindows) {
      val dotnetDir = createTempDirectory(temporaryDirectory, "dotnet")
      createDirectories(dotnetDir)
      val dotnetEnv = new DotnetEnvironment(dotnetDir)  // .closeWithCloser  The DLLs cannot be removed. They are still loaded.
      conf withDotnetAdapterDirectory Some(dotnetEnv.directory)
    }
    else conf
  }
}
