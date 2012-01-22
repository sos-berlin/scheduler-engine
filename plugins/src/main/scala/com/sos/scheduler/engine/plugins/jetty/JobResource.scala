package com.sos.scheduler.engine.plugins.jetty

import javax.inject.Inject
import javax.ws.rs._
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.noCache
import javax.ws.rs.core.{EntityTag, MediaType, Response, Variant}

@Path("/objects/job")
class JobResource @Inject()(jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId,
    @QueryParam("job") pathString: String) {

  private lazy val job = jobSubsystem.job(AbsolutePath.of(pathString))
  private lazy val jobTag = new EntityTag(job.getUuid.toString);

  @GET @Path("configuration")
  @Produces(Array(MediaType.TEXT_XML))
  def getJobConfiguration = Response.ok(job.getConfigurationXml).tag(jobTag).build()

  @GET @Path("description")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getJobLog = Response.ok(job.getDescription).tag(jobTag).build()

  @GET @Path("log.snapshot")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getJobLogSnapshot = {
    val file = job.getLog.getFile
    val variant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, schedulerEncoding.name())
    Response.ok(file, variant).cacheControl(noCache).build()
  }
}
