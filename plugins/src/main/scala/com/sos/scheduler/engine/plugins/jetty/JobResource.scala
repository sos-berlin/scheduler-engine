package com.sos.scheduler.engine.plugins.jetty

import scala.collection.JavaConversions._
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response, Variant}
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.{noCache, wrapXmlResponse}

@Path("/objects/{path:.+}.job")
class JobResource @Inject()(@PathParam("path") pathString: String,
    jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId) {

  //TODO Exceptions auf HTTP-Codes abbilden

  private lazy val job = jobSubsystem.job(AbsolutePath.of(pathString))

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(@PathParam("type") typeString: String) = {
    val contents = wrapXmlResponse(jobSubsystem.getVisibleNames map { name => <job name={name}/> })
    Response.ok(contents).cacheControl(noCache).build()
  }

  @Path("/log.snapshot")
  @GET
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getJobLog(@PathParam("type") typeString: String) = {
    val file = job.getLog.getFile
    //val tag = new EntityTag(schedulerInstanceId.getString +"."+file.length)  Datei kann während der Antwort wachsen
    val variant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, schedulerEncoding.name())
    Response.ok(file, variant).cacheControl(noCache).build()
  }

  //  @GET
  //  @Path("{path:.+}.job/description")
  //  @Produces(Array(MediaType.TEXT_PLAIN))
  //  def getJobLog(@PathParam("type") typeString: String, @PathParam("path") pathString: String) = {
  //    val content = scheduler.getJobSubsystem.job(AbsolutePath.of(pathString)).getDescription
  //    Response.ok(content).cacheControl(noCache).build()
  //  }
}

object JobResource {
}
