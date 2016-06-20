package com.sos.scheduler.engine.plugins.jetty.test

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests.UmlautJobPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files

/**
 * @author Joacim Zschimmer
 */
trait ProvideUmlautJob {
  this: ScalaSchedulerTest ⇒

  /**
   * Under Linux, TestEnvironment (JavaResourceResolver) cannot handle non-ASCII characters in filenames,
   * so we rename the file here when we need it unter Windows.
   */
  final def provideUmlautJob(): Unit = {
    assert(!isUnix)
    Files.move(testEnvironment.liveDirectory / "test-umlauts.job.txt", testEnvironment.fileFromPath(UmlautJobPath))
    instance[FolderSubsystem].updateFolders()
  }
}
