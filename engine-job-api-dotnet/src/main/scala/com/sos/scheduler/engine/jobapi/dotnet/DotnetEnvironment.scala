package com.sos.scheduler.engine.jobapi.dotnet

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.jobapi.dotnet.dlls.DotnetDlls
import java.nio.file.Files._
import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
final class DotnetEnvironment(baseTemporaryDirectory: Path) extends HasCloser {
  val directory = createTempDirectory(baseTemporaryDirectory, "dotnet")
  private val dlls = DotnetDlls.provideDlls(directory)

  onClose {
    (dlls :+ directory) foreach deleteIfExists
  }
}
