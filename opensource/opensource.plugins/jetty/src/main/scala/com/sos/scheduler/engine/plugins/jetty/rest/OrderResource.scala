package com.sos.scheduler.engine.plugins.jetty.rest

import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.{MediaType, Response}
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices._

@Path("order")
class OrderResource @Inject()(
    @QueryParam("jobChain") @DefaultValue("") jobChainPathString: String,
    @PathParam("orderId") @DefaultValue("") orderId: OrderId,
    orderSubsystem: OrderSubsystem,
    schedulerInstanceId: SchedulerInstanceId) {

  private lazy val jobChain = orderSubsystem.jobChain(JobChainPath.of(jobChainPathString))
  private lazy val order = jobChain.order(orderId)

  @GET @Path("log.snapshot")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getLogSnapshot = Response.ok(order.getLog.getFile, schedulerTextPlainVariant).cacheControl(noCache).build()
}
