package com.sos.scheduler.engine.plugins.jetty.rest

import javax.inject.Inject
import javax.ws.rs.core.MediaType._
import javax.ws.rs._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.rest.annotations.HtmlXsltResource
import javax.ws.rs.core.{UriInfo, Context, Response}

@Path("")
class RootResource @Inject()(schedulerInstanceId: SchedulerInstanceId) {
  @GET
  @Produces(Array(TEXT_HTML))
  @HtmlXsltResource(path="com/sos/scheduler/engine/plugins/jetty/rest/RootResource.xsl")
  def get(@Context u: UriInfo) = {
    val result = <engine/>
    Response.ok(result).build()
  }
}
