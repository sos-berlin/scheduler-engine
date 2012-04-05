package com.sos.scheduler.engine.plugins.jetty.rest.views

import com.sos.scheduler.engine.data.folder.AbsolutePath
import java.net.URI
import javax.ws.rs.core.UriBuilder
import org.codehaus.jackson.annotate.JsonProperty

class FolderView(_folderPath: AbsolutePath, _typeName: String, _entries: Iterable[FolderView.Entry]) {
  @JsonProperty def folderPath = _folderPath
  @JsonProperty def typeName = _typeName
  @JsonProperty def entries = _entries
}

object FolderView {
  def apply(names: Iterable[String], folderPath: AbsolutePath, typeName: String, baseUri: URI) = {
    def entry(name: String) = new Entry(name,
      UriBuilder.fromUri(baseUri).segment(typeName).queryParam(typeName, AbsolutePath.of(folderPath, name).toString).build())
    new FolderView(folderPath, typeName, names map entry)
  }

  case class Entry(name: String, uri: URI)
}
