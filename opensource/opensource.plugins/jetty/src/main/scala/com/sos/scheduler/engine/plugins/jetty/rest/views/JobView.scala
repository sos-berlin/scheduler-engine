package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.data.folder.AbsolutePath
import java.net.URI
import javax.ws.rs.core.UriBuilder

case class JobView(descriptionUri: URI, configurationUri: URI, logSnapshotUri: URI)

object JobView {
  def apply(path: AbsolutePath, jobUri: URI) = {
    def newUri() = UriBuilder.fromUri(jobUri)

    new JobView(
      descriptionUri = newUri().segment("description").build(),
      configurationUri = newUri().segment("configuration").build(),
      logSnapshotUri = newUri().segment("log.snapshot").build()
    )
  }
}