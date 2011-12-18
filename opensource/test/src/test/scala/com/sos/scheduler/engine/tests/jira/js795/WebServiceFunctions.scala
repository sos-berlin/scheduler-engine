package com.sos.scheduler.engine.tests.jira.js795

import javax.ws.rs.core.CacheControl

object WebServiceFunctions {
  val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }
}