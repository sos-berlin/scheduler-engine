package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import java.net.URI
import javax.ws.rs.core.UriBuilder

case class JobView(path: AbsolutePath, jobUri: URI) {
  private def u = UriBuilder.fromUri(jobUri)

  lazy val descriptionUri = u.path("description").build()
  lazy val configurationUri = u.path("configuration").build()
  lazy val logSnapshotUri = u.path("log.snapshot").build()
}
