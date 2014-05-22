package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.kernel.job.{TaskNotFoundException, TaskSubsystem}
import com.sun.jersey.spi.resource.Singleton
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status.BAD_REQUEST

@Path("tasks")
@Singleton
final class TasksService @Inject private(taskSubsystem: TaskSubsystem) {
  @GET
  @Path("{taskId}/log")
  @Produces(Array(TEXT_PLAIN))
  def get(@PathParam("taskId") taskIdString: String) = {
    val taskId =
      try TaskId(taskIdString.toInt)
      catch { case e: Exception =>  throw new WebApplicationException(e, BAD_REQUEST) }

    // Wir lesen die ganze Datei auf einmal in den String, damit sie unter Windows nicht gesperrt bleibt. Hoffen wir, dass nicht zu viele zu große Tasks-Logs gleichzeitig gelesen werden.
    try taskSubsystem.taskLog(taskId)
    catch { case e: TaskNotFoundException => throw new WebApplicationException(e, BAD_REQUEST) }
  }
}
