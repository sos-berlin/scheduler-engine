package com.sos.scheduler.engine.plugins.jetty.rest

import javax.inject.Inject
import javax.ws.rs.core.MediaType._
import javax.ws.rs._
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices._
import com.sos.scheduler.engine.plugins.jetty.rest.annotations.HtmlXsltResource
import javax.ws.rs.core.Response.Status.NOT_FOUND
import javax.ws.rs.core.{UriInfo, Context, EntityTag, Response}

@Path("job")
class JobResource @Inject()(jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId,
    @QueryParam("job") @DefaultValue("") pathString: String) {

  private lazy val path = AbsolutePath.of(pathString)

  private lazy val job =
    try jobSubsystem.job(path)
    catch {
      case x: CppException if x.getCode == "SCHEDULER-161" => throw new WebApplicationException(x, NOT_FOUND)
    }

  private lazy val jobTag = new EntityTag(job.getUuid.toString);

  @GET
  @Produces(Array(TEXT_HTML))
  @HtmlXsltResource(path= "com/sos/scheduler/engine/plugins/jetty/rest/JobResource.xsl")
  def getRoot(@Context u: UriInfo) = {
    val uri = u.getBaseUriBuilder.path("job").build()
    val result = <job job={path.toString} uri={uri.toString}/>
    Response.ok(result).tag(jobTag).build()
  }

  @GET @Path("configuration")
  @Produces(Array(TEXT_XML))
  def getConfiguration = Response.ok(job.getConfigurationXmlBytes).tag(jobTag).build()

  @GET @Path("description")
  @Produces(Array(TEXT_PLAIN))
  def getDescription = Response.ok(job.getDescription, textPlainVariant).tag(jobTag).build()

  @GET @Path("log.snapshot")
  @Produces(Array(TEXT_PLAIN))
  def getLogSnapshot = Response.ok(job.getLog.getFile, schedulerTextPlainVariant).cacheControl(noCache).build()
}
