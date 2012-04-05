package com.sos.scheduler.engine.plugins.jetty.rest

import javax.inject.Inject
import javax.ws.rs._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.rest.annotations.HtmlXsltResource
import com.sos.scheduler.engine.plugins.jetty.rest.views.RootView
import javax.ws.rs.core._

@Path("")
class RootResource @Inject()(schedulerInstanceId: SchedulerInstanceId) {
  private lazy val tag = new EntityTag(schedulerInstanceId.asString)

  @GET
  @Produces(Array(MediaType.APPLICATION_JSON))
  @HtmlXsltResource(path="com/sos/scheduler/engine/plugins/jetty/rest/RootResource.xsl")
  def get(@Context u: UriInfo) = {
    val result = RootView(u.getBaseUri)
    Response.ok(result).tag(tag).build()
  }
}
