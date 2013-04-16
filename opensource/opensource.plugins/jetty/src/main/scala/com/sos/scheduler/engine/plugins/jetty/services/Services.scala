package com.sos.scheduler.engine.plugins.jetty.services

object Services {
  val services: Iterable[Class[_]] = Iterable(
    classOf[CommandService],
    classOf[EventsService],
    classOf[FolderService],
    classOf[JobService],
    classOf[JobsService],
    classOf[RootService])
}
