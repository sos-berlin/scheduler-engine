package com.sos.scheduler.engine.taskserver.dotnet

import com.sos.scheduler.engine.common.process.windows.WindowsProcess
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.taskserver.dotnet.dlls.DotnetDlls
import java.nio.file.Files._
import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
final class DotnetEnvironment(val directory: Path) extends HasCloser {

  WindowsProcess.makeDirectoryAccessibleForEverybody(directory)
  private val files = DotnetDlls.provideDlls(directory)

  onClose {
    files foreach deleteIfExists
    deleteIfExists(directory)
  }

  override def toString = s"DotnetEnvironment $directory"
}
