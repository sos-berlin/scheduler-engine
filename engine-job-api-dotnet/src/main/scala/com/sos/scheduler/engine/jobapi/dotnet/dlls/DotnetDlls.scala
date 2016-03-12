package com.sos.scheduler.engine.jobapi.dotnet.dlls

import com.sos.scheduler.engine.common.utils.JavaResource
import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
object DotnetDlls {
  private val DllsResourcePath = JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls")
  private[dotnet] val DllNames = Set("readme.txt")  // FIXME Replace with DLL names

  def provideDlls(directory: Path): Set[Path] =
    DllsResourcePath.copyToFiles(DllNames, directory).toSet
}
