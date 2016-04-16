package com.sos.scheduler.engine.jobapi.dotnet.dlls

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.utils.JavaResource
import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
object DotnetDlls {

  private val logger = Logger(getClass)

  private[dotnet] val DllsResourcePaths = {
    val jni4netDlls = Set(
      sys.props.get("sun.arch.data.model") match {
        case Some("64") ⇒ JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n.w64.v40-0.8.8.0.dll")
        case Some("32") ⇒ JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n.w32.v40-0.8.8.0.dll")
        case o ⇒ sys.error(s"Unknown Java property sun.arch.data.model=$o")
      },
      JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/jni4net.n-0.8.8.0.dll"))
    val generatedDlls = Set(
      JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/com.sos-berlin.jobscheduler.dotnet.adapter.dll"),
      JavaResource("com/sos/scheduler/engine/jobapi/dotnet/dlls/com.sos-berlin.jobscheduler.engine-job-api.j4n.dll"))
    jni4netDlls ++ generatedDlls
  }

  def provideDlls(directory: Path): Set[Path] =
    for (resource ← DllsResourcePaths) yield {
      val file = directory resolve resource.simpleName
      logger.debug(s"Providing $file")
      resource.copyToFile(file)
      file
    }
}
