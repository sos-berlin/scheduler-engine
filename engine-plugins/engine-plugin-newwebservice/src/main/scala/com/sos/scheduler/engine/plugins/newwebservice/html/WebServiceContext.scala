package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.client.web.SchedulerUris

/**
  * @author Joacim Zschimmer
  */
final class WebServiceContext(
  val uris: SchedulerUris = SchedulerUris("/"),
  val htmlEnabled: Boolean = false)
