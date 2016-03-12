package com.sos.scheduler.engine.jobapi.dotnet

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import java.nio.file.Files.{createTempDirectory, delete}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class DotnetEnvironmentTest extends FreeSpec {

  "DotnetEnvironment provides files and cleans up after use" in {
    val baseDir = createTempDirectory("DotnetEnvironmentTest-")  // Double nesting of directories to get a clear test
    val dotnetEnvironment = new DotnetEnvironment(baseDir)
    assert(baseDir.pathSet == Set(dotnetEnvironment.directory))
    assert(dotnetEnvironment.directory.pathSet == (dlls.DotnetDlls.DllNames map dotnetEnvironment.directory.resolve))
    dotnetEnvironment.close()
    assert(baseDir.pathSet == Set())
    delete(baseDir)
  }
}
