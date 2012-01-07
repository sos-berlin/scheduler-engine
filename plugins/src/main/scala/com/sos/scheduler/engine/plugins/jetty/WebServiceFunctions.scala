package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.core.CacheControl

object WebServiceFunctions {
  val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }
}