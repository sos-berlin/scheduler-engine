package com.sos.scheduler.engine.jobapi.dotnet.dlls

import com.sos.scheduler.engine.common.utils.JavaResource
import java.nio.file.{CopyOption, Path}
import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
object DotnetDlls {
  private val DllsResourcePath = JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls")
  private val DllNames = List("jni4net.dll")

  def provideDlls(directory: Path, copyOptions: CopyOption*): immutable.Seq[Path] =
    DllsResourcePath.copyToFiles(DllNames, directory, copyOptions: _*)
}
