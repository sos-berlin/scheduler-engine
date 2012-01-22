package com.sos.scheduler.engine.plugins.jetty

object JettyPluginConfiguration {
  val adminstratorRoleName = "administrator"
  val prefixPath = "/JobScheduler/engine"
  val cppPrefixPath = ""

  val gzipContentTypes = List(
    "application/javascript",
    "application/xhtml+xml",
    "image/svg+xml",
    "text/css",
    "text/html",
    "text/plain",
    "text/xml")
}
