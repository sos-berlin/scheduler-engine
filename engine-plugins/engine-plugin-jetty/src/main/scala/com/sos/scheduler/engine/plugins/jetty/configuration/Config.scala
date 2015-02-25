package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.utils.JavaResource

object Config {
  val contextPath = "/jobscheduler"
  val ResourceBaseResource = JavaResource("com/sos/scheduler/engine/plugins/jetty/empty")
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
