package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import javax.ws.rs.core.{EntityTag, MediaType, Response}
import javax.ws.rs._
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.Constants._
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.noCache

@Path("/objects/job")
class JobResource @Inject()(jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId,
    @QueryParam("job") pathString: String) {

  private lazy val job = jobSubsystem.job(AbsolutePath.of(pathString))
  private lazy val jobTag = new EntityTag(job.getUuid.toString);

  @GET @Path("configuration")
  @Produces(Array(MediaType.TEXT_XML))
  def getConfiguration = Response.ok(job.getConfigurationXmlBytes).tag(jobTag).build()

  @GET @Path("description")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getDescription = Response.ok(job.getDescription, textPlainVariant).tag(jobTag).build()

  @GET @Path("log.snapshot")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getLogSnapshot = Response.ok(job.getLog.getFile, schedulerTextPlainVariant).cacheControl(noCache).build()
}
