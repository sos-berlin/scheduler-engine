package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.{noCache, schedulerTextPlainVariant, textPlainVariant}
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status.BAD_REQUEST
import javax.ws.rs.core._

@Path("job")
class JobService @Inject private(
    jobSubsystem: JobSubsystemClient,
    schedulerInstanceId: SchedulerInstanceId,
    @QueryParam("job") @DefaultValue("") pathString: String)
{
  private lazy val path = JobPath.makeAbsolute(pathString)

  private lazy val job =
    try jobSubsystem.job(path)
    catch {
      case x: CppException if x.getCode == "SCHEDULER-161" => throw new WebApplicationException(x, BAD_REQUEST)
    }

  private lazy val jobTag = new EntityTag(job.uuid.toString)

  @GET
  @Produces(Array(APPLICATION_JSON))
  def get(@Context u: UriInfo) = JobView(path, UriBuilder.fromUri(u.getBaseUri).path("job").queryParam("job", path.string).build())

  @GET @Path("configuration")
  @Produces(Array(TEXT_XML))
  def getConfiguration = Response.ok(job.configurationXmlBytes).tag(jobTag).build()

  @GET @Path("description")
  @Produces(Array(TEXT_PLAIN))
  def getDescription = Response.ok(job.description, textPlainVariant).tag(jobTag).build()

  @GET @Path("log")
  @Produces(Array(TEXT_PLAIN))
  def getLogSnapshot = Response.ok(job.log.file, schedulerTextPlainVariant).cacheControl(noCache).build()
}
