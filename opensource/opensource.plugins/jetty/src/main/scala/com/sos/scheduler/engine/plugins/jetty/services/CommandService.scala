package com.sos.scheduler.engine.plugins.jetty.services

import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.MediaType
import javax.ws.rs.core.Response
import com.sos.scheduler.engine.plugins.jetty.services.WebServices.noCache
import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor

@Path("command")
class CommandService @Inject()(xmlCommandExecutor: SchedulerXmlCommandExecutor) {
  @POST
  @Consumes(Array(MediaType.TEXT_XML))
  @Produces(Array(MediaType.TEXT_XML))
  def post(command: String) = executeCommand(command)

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def get(@DefaultValue("") @QueryParam("command") command: String) = executeCommand(command)

  private def executeCommand(command: String) = {
    val resultXml = xmlCommandExecutor.uncheckedExecuteXml(command)
    Response.ok(resultXml).cacheControl(noCache).build()
  }
}
