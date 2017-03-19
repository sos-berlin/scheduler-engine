package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.jobscheduler.common.sprayutils.html.WebServiceContext
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext._

/**
  * @author Joacim Zschimmer
  */
final class SchedulerWebServiceContext  (
  val htmlEnabled: Boolean = false,
  val uris: SchedulerUris = SchedulerUris("/"))
extends WebServiceContext {

  def toWebjarUri(path: String) = uris / s"api/frontend/webjars/$path"

  def staticFilesResource = StaticPackage
}

object SchedulerWebServiceContext {
  private val StaticPackage = JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/")
}
