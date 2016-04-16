package com.sos.scheduler.engine.jobapi.dotnet.dlls

import com.sos.scheduler.engine.common.utils.JavaResource
import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
object DotnetDlls {
  private val Jni4net64Dll = JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n.w64.v40-0.8.8.0.dll")
  private val Jni4net32Dll = JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n.w32.v40-0.8.8.0.dll")
  private[dotnet] val DllsResourcePaths = Set(
    if (sys.props("sun.arch.data.model") == "32") Jni4net32Dll else Jni4net64Dll,
    JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n-0.8.8.0.dll"),
    JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/com.sos-berlin.jobscheduler.dotnet.adapter.dll"),
    JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/com.sos-berlin.jobscheduler.engine-job-api.j4n.dll"))

  def provideDlls(directory: Path): Set[Path] =
    for (resource ← DllsResourcePaths) yield {
      val file = directory resolve resource.simpleName
      resource.copyToFile(file)
      file
    }
}
