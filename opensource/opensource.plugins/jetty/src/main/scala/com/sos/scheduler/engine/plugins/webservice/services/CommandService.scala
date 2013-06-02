package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.plugins.jetty.SchedulerSecurityRequest
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.noCache
import javax.inject.Inject
import javax.servlet.http.HttpServletRequest
import javax.ws.rs._
import javax.ws.rs.core.{Context, MediaType, Response}

@Path("command")
final class CommandService @Inject private(xmlCommandExecutor: SchedulerXmlCommandExecutor) {
  @POST
  @Consumes(Array(MediaType.TEXT_XML))
  @Produces(Array(MediaType.TEXT_XML))
  def post(
      command: String,
      @Context request: HttpServletRequest) =
    executeCommandWithSecurityLevel(command, SchedulerSecurityRequest.securityLevel(request))

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(
      @DefaultValue("") @QueryParam("command") command: String,
      @Context request: HttpServletRequest) =
    executeCommandWithSecurityLevel(command, SchedulerSecurityRequest.securityLevel(request))

  private def executeCommandWithSecurityLevel(command: String, securityLevel: SchedulerSecurityLevel) = {
    val resultXml: String = xmlCommandExecutor.uncheckedExecuteXml(command, securityLevel)
    Response.ok(resultXml).cacheControl(noCache).build()
  }
}
