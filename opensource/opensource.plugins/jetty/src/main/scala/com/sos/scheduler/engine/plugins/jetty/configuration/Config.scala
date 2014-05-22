package com.sos.scheduler.engine.plugins.jetty.configuration

object Config {
  val contextPath = "/jobscheduler"
  val resourceBaseURL = getClass.getResource("/com/sos/scheduler/engine/web")
  val enginePrefixPath = "/engine"
  val cppPrefixPath = "/engine-cpp"
  val administratorRoleName = "administrator"

  //  val gzipContentTypes = List(
  //    "application/javascript",
  //    "application/xhtml+xml",
  //    "image/svg+xml",
  //    "text/css",
  //    "text/html",
  //    "text/javascript",
  //    "text/plain",
  //    "text/xml")
}
