package com.sos.scheduler.engine.plugins.jetty.rest

import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response, Variant}
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.rest.WebServiceFunctions.noCache
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, OrderId}

@Path("/objects/{path:.+}.job_chain/orders/{orderId}")
class OrderResource @Inject()(@PathParam("path") jobChainPathString: String, @PathParam("orderId") orderId: OrderId,
    orderSubsystem: OrderSubsystem, schedulerInstanceId: SchedulerInstanceId) {

  private lazy val jobChain = orderSubsystem.jobChain(AbsolutePath.of(jobChainPathString))
  private lazy val order = jobChain.order(orderId)

  @Path("{path:.+}.job_chain/orders/{orderId}/log")
  @GET
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getOrderLog(@PathParam("type") typeString: String, @PathParam("path") pathString: String) = {
    val file = order.getLog.getFile
    //val tag = new EntityTag(schedulerInstanceId.getString +"."+file.length)  Datei kann während der Antwort wachsen
    val variant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, schedulerEncoding.name())
    Response.ok(file, variant).cacheControl(noCache).build()
  }
}
