package com.sos.scheduler.engine.plugins.jetty.services

import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core._

@Path("")
class RootService @Inject()(schedulerInstanceId: SchedulerInstanceId) {
  private lazy val tag = new EntityTag(schedulerInstanceId.asString)

  @GET
  @Produces(Array(MediaType.APPLICATION_JSON))
  def get(@Context u: UriInfo) = {
    val result = RootView(u.getBaseUri)
    Response.ok(result).tag(tag).build()
  }
}
