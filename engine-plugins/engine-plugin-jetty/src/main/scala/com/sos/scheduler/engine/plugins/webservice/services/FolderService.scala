package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.data.filebased.{AbsolutePath, FileBasedType}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.noCache
import javax.inject.{Inject, Singleton}
import javax.servlet.http.HttpServletRequest
import javax.ws.rs._
import javax.ws.rs.core.Response.Status.BAD_REQUEST
import javax.ws.rs.core._

@Path("folder")
@Singleton
class FolderService @Inject private(folderSubsystem: FolderSubsystem) {
  @GET
  @Produces(Array(MediaType.APPLICATION_JSON))
  def get(@QueryParam("folder") @DefaultValue("") folderPathString: String,
          @QueryParam("type") @DefaultValue("") typeName: String,
          @Context request: HttpServletRequest, @Context u: UriInfo) = {
    val folderPath = AbsolutePath.of(folderPathString)
    val v = view(folderPath, typeName, u)
    Response.ok(v).cacheControl(noCache).build()
  }

  private def view(folderPath: AbsolutePath, typeName: String, u: UriInfo) = {
    val typ = FileBasedType.values find { _.toString.toLowerCase == typeName.toLowerCase } getOrElse FileBasedType.fromCppName(typeName)
    try FolderView(folderSubsystem.names(folderPath, typ), folderPath, typeName, u.getBaseUri)
    catch {
      case x: CppException if x.getCode == "SCHEDULER-161" ⇒ throw new WebApplicationException(x, BAD_REQUEST)
    }
  }
}
