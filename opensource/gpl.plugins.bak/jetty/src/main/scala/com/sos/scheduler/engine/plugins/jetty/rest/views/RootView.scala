package com.sos.scheduler.engine.plugins.jetty.rest.views

import java.net.URI
import javax.ws.rs.core.UriBuilder
import com.sos.scheduler.engine.kernel.folder.FolderTypeName

case class RootView(baseUri: URI) {
  private def u = UriBuilder.fromUri(baseUri)

  //lazy val configurationUri = u.path("configuration").build()
  lazy val logUri = u.path("log").build()
  lazy val folderUris = FolderTypeName.values() map { o => o -> folderUri(o) }

  private def folderUri(typeName: FolderTypeName) = u.path("folder").queryParam("type", typeName.name()).build()

//  lazy val jobFolderUri = u.path("folder").queryParam("type", "job").build()
//  lazy val jobChainFolderUri = u.path("folder").queryParam("type", "job_chain").build()
//  lazy val processClassFolderUri = u.path("folder").queryParam("type", "process_class").build()
}
