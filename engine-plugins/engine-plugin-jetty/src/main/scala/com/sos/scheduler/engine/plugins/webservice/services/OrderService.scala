package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.{noCache, schedulerTextPlainVariant}
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}

@Path("order")
class OrderService @Inject private(
    @QueryParam("jobChain") @DefaultValue("") jobChainPathString: String,
    @PathParam("orderId") @DefaultValue("") orderId: OrderId,
    orderSubsystem: OrderSubsystem,
    schedulerInstanceId: SchedulerInstanceId) {

  private lazy val jobChain = orderSubsystem.jobChain(JobChainPath(jobChainPathString))
  private lazy val order = jobChain.order(orderId)

  @GET @Path("log")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getLogSnapshot = Response.ok(order.log.file, schedulerTextPlainVariant).cacheControl(noCache).build()
}
