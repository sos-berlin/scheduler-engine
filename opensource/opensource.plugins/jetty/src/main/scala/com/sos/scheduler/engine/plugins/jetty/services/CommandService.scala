package com.sos.scheduler.engine.plugins.jetty.services

import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor
import com.sos.scheduler.engine.plugins.jetty.SchedulerSecurityRequest
import com.sos.scheduler.engine.plugins.jetty.services.WebServices.noCache
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
    executeCommandWithSecurityLevel(command, request)

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(
      @DefaultValue("") @QueryParam("command") command: String,
      @Context request: HttpServletRequest) =
    executeCommandWithSecurityLevel(command, request)

  private def executeCommandWithSecurityLevel(command: String, request: HttpServletRequest ) = {
    val securityLevel = SchedulerSecurityRequest.securityLevel(request)
    val clientHost = Option(request.getHeader("x-forwarded-for")) map { _ takeWhile { _ != ',' } } getOrElse request.getRemoteHost
    val resultXml: String = xmlCommandExecutor.uncheckedExecuteXml(command, securityLevel, clientHost)
    Response.ok(resultXml).cacheControl(noCache).build()
  }
}
