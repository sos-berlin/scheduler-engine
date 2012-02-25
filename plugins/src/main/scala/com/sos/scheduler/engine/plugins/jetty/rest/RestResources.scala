package com.sos.scheduler.engine.plugins.jetty.rest

object RestResources {
  /** REST-Ressourcen. */
  val resources: Iterable[Class[_]] = Iterable(
    classOf[CommandResource],
    classOf[FolderResource],
    classOf[JobResource],
    classOf[JobsResource],
    classOf[RootResource])
}
