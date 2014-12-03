package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.common.scalautil.ScalaUtils._

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
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
