package com.sos.scheduler.engine.plugins.jetty.rest.views

import java.net.URI
import javax.ws.rs.core.UriBuilder
import com.sos.scheduler.engine.kernel.folder.FolderTypeName
import org.codehaus.jackson.map.annotate.JsonSerialize

@JsonSerialize(using=classOf[RootViewSerializer])
trait RootView {
  val logUri: URI
  val folders: Iterable[(String,URI)]
}

object RootView {
  def apply(baseUri: URI) = {
    def newUri() = UriBuilder.fromUri(baseUri)
    def folderUri(typeName: FolderTypeName) = newUri().path("folder").queryParam("type", typeName.name()).build()

    new RootView {
      val logUri = newUri().path("log").build()
      val folders = FolderTypeName.values.toSeq map { o => o.name -> folderUri(o) }
    }
  }
}