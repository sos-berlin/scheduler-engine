package com.sos.scheduler.engine.plugins.webservice.services

import com.fasterxml.jackson.databind.annotation.JsonSerialize
import com.sos.scheduler.engine.data.filebased.FileBasedType
import java.net.URI
import javax.ws.rs.core.UriBuilder

@JsonSerialize(using=classOf[RootViewSerializer])
trait RootView {
  val logUri: URI
  val folders: Iterable[(String, URI)]
}

object RootView {
  def apply(baseUri: URI) = {
    def newUri() = UriBuilder.fromUri(baseUri)
    def folderUri(typeName: FileBasedType) = newUri().path("Folder").queryParam("type", typeName.name()).build()

    new RootView {
      val logUri = newUri().path("log").build()
      val folders = FileBasedType.values.toSeq map { o => o.name -> folderUri(o) }
    }
  }
}
