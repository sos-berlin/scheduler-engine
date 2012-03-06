package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import java.net.URI
import javax.ws.rs.core.UriBuilder

case class FolderView(names: Iterable[String], folderPath: AbsolutePath, typeName: String, baseUri: URI) {
  val entries = names map Entry.apply

  case class Entry(name: String) {
    def uri = UriBuilder.fromUri(baseUri).path(typeName).queryParam(typeName, AbsolutePath.of(folderPath, name).toString).build()
  }
}
