package com.sos.scheduler.engine.plugins.jetty.rest

object RestResources {
  /** REST-Ressourcen. */
  val resources: Iterable[Class[_]] = Iterable(
    classOf[CommandResource],
    classOf[FoldersResource],
    classOf[JobResource],
    classOf[JobsResource])
}
