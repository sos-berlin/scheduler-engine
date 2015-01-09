package com.sos.scheduler.engine.taskserver.configuration

import com.sos.scheduler.engine.data.agent.RemoteTaskId
import java.net.InetSocketAddress

/**
 * @author Joacim Zschimmer
 */
final case class StartConfiguration(
  remoteTaskId: RemoteTaskId,
  controllerAddress: InetSocketAddress,
  usesApi: Boolean,
  javaOptions: String,
  javaClasspath: String)
