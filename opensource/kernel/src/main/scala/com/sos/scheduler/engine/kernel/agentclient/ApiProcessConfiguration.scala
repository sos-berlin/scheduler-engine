package com.sos.scheduler.engine.kernel.agentclient

import com.sos.scheduler.engine.kernel.cppproxy.Api_process_configurationC
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import java.net.URI

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
  remoteSchedulerUri: URI,
  hasApi: Boolean,
  javaOptions: String,
  javaClasspath: String) {

  def startRemoteTaskXmlElem(schedulerApiTcpPort: Int) =
    <remote_scheduler.start_remote_task
      tcp_port={schedulerApiTcpPort.toString}
      kind={if (!hasApi) "process" else null}
      java_options={javaOptions.trim substitute "" -> null}
      java_classpath={javaClasspath.trim substitute "" -> null}/>
}

object ApiProcessConfiguration {
  def apply(c: Api_process_configurationC) = new ApiProcessConfiguration(
    remoteSchedulerUri = new URI(c._remote_scheduler_address),
    hasApi = c._has_api,
    javaOptions = c._java_options,
    javaClasspath = c._java_classpath)
}
