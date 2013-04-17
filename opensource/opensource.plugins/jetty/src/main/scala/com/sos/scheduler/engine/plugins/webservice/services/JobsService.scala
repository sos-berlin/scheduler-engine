package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.{noCache, wrapXmlResponse}
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}
import scala.collection.JavaConversions._

@Path("jobs")
class JobsService @Inject()(jobSubsystem: JobSubsystem) {
  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get() = {
    val contents = wrapXmlResponse(jobSubsystem.getVisibleNames map { name => <job name={name}/> })
    Response.ok(contents).cacheControl(noCache).build()
  }
}
