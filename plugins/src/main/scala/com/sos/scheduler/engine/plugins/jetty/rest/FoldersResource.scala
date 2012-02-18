package com.sos.scheduler.engine.plugins.jetty.rest

import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.folder.{AbsolutePath, FolderSubsystem}
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.noCache
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.Response.Status.NOT_FOUND
import javax.ws.rs.core._
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.plugins.jetty.rest.annotations.HtmlXsltResource

@Path("folders")
class FoldersResource @Inject()(folderSubsystem: FolderSubsystem) {
  @GET
  @Produces(Array(MediaType.TEXT_XML, MediaType.TEXT_HTML))
  @HtmlXsltResource(path="com/sos/scheduler/engine/plugins/jetty/rest/FoldersResource.xsl")
  def getXml(@QueryParam("folder") @DefaultValue("") pathString: String,
             @QueryParam("type") @DefaultValue("") typeName: String,
             @Context u: UriInfo) = {
    val path = AbsolutePath.of(pathString)
    val result = namesAsXml(path, typeName, u)
    Response.ok(result).cacheControl(noCache).build()
  }

  private def namesAsXml(folderPath: AbsolutePath, typeName: String, u: UriInfo) = {
    def toXml(name: String) = {
      val uri = u.getBaseUriBuilder.path(typeName).queryParam("job", AbsolutePath.of(folderPath, name).toString).build()
      <name name={name} uri={uri.toString}/>
    }
    val contents = names(folderPath, typeName) map toXml
    <names folder={folderPath.withTrailingSlash} type={typeName}>{contents}</names>
  }

  private def names(path: AbsolutePath, typeName: String) = {
    try folderSubsystem.names(path, typeName)
    catch {
      case x: CppException if x.getCode == "SCHEDULER-161" =>
        throw new WebApplicationException(x, NOT_FOUND)
    }
  }
}
