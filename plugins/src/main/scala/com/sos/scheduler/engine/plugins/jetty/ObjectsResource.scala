package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs._
import javax.ws.rs.core.MediaType
import javax.inject.Inject
import scala.collection.JavaConversions._
import scala.xml.Elem
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.folder.AbsolutePath

@Path("objects")
class ObjectsResource @Inject()(scheduler: Scheduler) {
  import ObjectsResource._

  @GET
  @Path("{folderPath:.+}/*.job")
  @Produces(Array(MediaType.TEXT_XML))
  def get(@PathParam("type") typeString: String, @PathParam("folderPath") folderPathString: String) =
    wrapXmlResponse(scheduler.getJobSubsystem.getVisibleNames map { name => <job name={name}/> })

  @GET
  @Path("{path:.+}.job/log")
  @Produces(Array(MediaType.TEXT_PLAIN))
  def getJobLog(@PathParam("type") typeString: String, @PathParam("path") pathString: String) = {
    val jobPath = new AbsolutePath("/"+pathString)
    scheduler.getJobSubsystem.job(jobPath).getLog.getFile
  }
}

object ObjectsResource {
  private def wrapXmlResponse(e: Seq[Elem]) = <scheduler>{e}</scheduler>
}
