package com.sos.scheduler.engine.plugins.jetty.services

import scala.collection.JavaConversions._
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.services.WebServices.{noCache, wrapXmlResponse}

@Path("jobs")
class JobsService @Inject()(jobSubsystem: JobSubsystem) {
  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get() = {
    val contents = wrapXmlResponse(jobSubsystem.getVisibleNames map { name => <job name={name}/> })
    Response.ok(contents).cacheControl(noCache).build()
  }
}
