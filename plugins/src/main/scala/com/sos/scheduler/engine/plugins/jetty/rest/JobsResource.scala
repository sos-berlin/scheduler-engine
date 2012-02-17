package com.sos.scheduler.engine.plugins.jetty.rest

import scala.collection.JavaConversions._
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.{noCache, wrapXmlResponse}

@Path("/jobs")
class JobsResource @Inject()(jobSubsystem: JobSubsystem) {
  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get() = {
    val contents = wrapXmlResponse(jobSubsystem.getVisibleNames map { name => <job name={name}/> })
    Response.ok(contents).cacheControl(noCache).build()
  }
}
