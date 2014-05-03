package com.sos.scheduler.engine.plugins.jetty.configuration

import com.google.common.io.Resources.getResource

object Config {
  val contextPath = "/jobscheduler"
  val resourceBaseResourcePath = "com/sos/scheduler/engine/web"
  val resourceBaseURL = getResource(resourceBaseResourcePath)
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
