package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.core.MediaType
import javax.ws.rs._
import javax.inject.Inject
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.kernel.Scheduler

@Path("objects")
class ObjectsResource @Inject()(scheduler: Scheduler) {
  @GET
  @Path("jobs/{path:.+}/")
  @Produces(Array(MediaType.TEXT_XML))
  def get() = {
    val jobs = scheduler.getJobSubsystem.getVisibleNames map { name => <job name={name}/> }
    <scheduler>{jobs}</scheduler>
  }

//  @GET
//  @Path("{path:.+}.{type:[a-z_]+}/log")
//  @Produces(Array(MediaType.TEXT_XML))
//  def get(@DefaultValue("false") @QueryParam("snapshort") snapshot: Boolean) = {
//
//  }
}