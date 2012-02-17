package com.sos.scheduler.engine.plugins.jetty.rest

import com.sos.scheduler.engine.kernel.folder.{AbsolutePath, FolderSubsystem}
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.noCache
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}
import scala.collection.JavaConversions._

@Path("folders")
class FoldersResource @Inject()(folderSubsystem: FolderSubsystem) {
  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(@QueryParam("path") @DefaultValue("") pathString: String,
          @QueryParam("type") @DefaultValue("") typeName: String) = {
    //TODO Bei Fehler SCHEDULER-161 404 liefern.
    val path = AbsolutePath.of(pathString)
    val contents = folderSubsystem.names(path, typeName) map { o => <name name={o}/> }
    val result = <names path={path.withTrailingSlash} type={typeName}>{contents}</names>
    Response.ok(result).cacheControl(noCache).build()
  }
}
