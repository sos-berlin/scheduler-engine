package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import javax.inject.{Singleton, Inject}
import javax.ws.rs._
import javax.ws.rs.core._

@Path("")
@Singleton
final class RootService @Inject private(schedulerInstanceId: SchedulerInstanceId) {
  private lazy val tag = new EntityTag(schedulerInstanceId.string)

  @GET
  @Produces(Array(MediaType.APPLICATION_JSON))
  def get(@Context u: UriInfo) = {
    val result = RootView(u.getBaseUri)
    Response.ok(result).tag(tag).build()
  }
}
