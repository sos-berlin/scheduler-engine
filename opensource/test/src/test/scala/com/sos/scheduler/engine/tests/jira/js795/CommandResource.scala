package com.sos.scheduler.engine.tests.jira.js795

import com.sos.scheduler.engine.kernel.Scheduler
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.MediaType
import javax.ws.rs.core.{CacheControl, Response}

@Path("command")
class CommandResource @Inject()(scheduler: Scheduler) {
  import CommandResource._

  @POST
  @Consumes(Array(MediaType.TEXT_XML))
  @Produces(Array(MediaType.TEXT_XML))
  def post(command: String) = executeCommand(command)

  @GET
  @Consumes(Array(MediaType.TEXT_XML))
  @Produces(Array(MediaType.TEXT_XML))
  def get(@DefaultValue("") @QueryParam("command") command: String) = executeCommand(command)

  private def executeCommand(command: String) = {
    val resultXml = scheduler.uncheckedExecuteXml(command) + CommandServlet.testString
    Response.ok(resultXml).cacheControl(noCache).build()
  }
}

object CommandResource {
  private val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }
}
