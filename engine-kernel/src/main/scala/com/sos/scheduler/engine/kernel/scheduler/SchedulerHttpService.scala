package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.kernel.cppproxy.{SpoolerC, HttpResponseC}
import com.sos.scheduler.engine.kernel.http.{SchedulerHttpResponse, SchedulerHttpRequest}
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import javax.inject.{Inject, Singleton}

@Singleton
final class SchedulerHttpService @Inject private(cppProxy: SpoolerC) {

  /** @return [[com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC#close]] MUSS aufgerufen werden! */
  def executeHttpRequest (request: SchedulerHttpRequest, response: SchedulerHttpResponse): HttpResponseC =
    cppProxy.java_execute_http(request, response)

  def executeHttpRequestWithSecurityLevel(request: SchedulerHttpRequest, response: SchedulerHttpResponse, securityLevel: SchedulerSecurityLevel) =
    cppProxy.java_execute_http_with_security_level(request, response, securityLevel.cppName)
}
