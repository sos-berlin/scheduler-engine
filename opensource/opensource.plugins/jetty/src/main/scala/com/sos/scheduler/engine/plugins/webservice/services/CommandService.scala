package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.noCache
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.MediaType
import javax.ws.rs.core.Response

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
