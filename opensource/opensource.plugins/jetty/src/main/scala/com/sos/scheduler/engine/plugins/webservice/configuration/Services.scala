package com.sos.scheduler.engine.plugins.webservice.configuration

import com.sos.scheduler.engine.plugins.webservice.services._

object Services {
  val services: Iterable[Class[_]] = Iterable(
    classOf[CommandService],
    classOf[EventsService],
    classOf[FolderService],
    classOf[JobService],
    classOf[JobsService])
}
