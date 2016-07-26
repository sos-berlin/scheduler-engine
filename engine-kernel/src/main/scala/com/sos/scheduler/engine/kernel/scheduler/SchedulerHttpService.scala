package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{HttpResponseC, SpoolerC}
import com.sos.scheduler.engine.kernel.http.{SchedulerHttpRequest, SchedulerHttpResponse}
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import javax.inject.{Inject, Singleton}

@Singleton
final class SchedulerHttpService @Inject private(cppProxy: SpoolerC)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  /** @return [[com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC#close]] MUSS aufgerufen werden! */
  def executeHttpRequest (request: SchedulerHttpRequest, response: SchedulerHttpResponse): HttpResponseC =
    inSchedulerThread {
      cppProxy.java_execute_http(request, response)
    }

  def executeHttpRequestWithSecurityLevel(request: SchedulerHttpRequest, response: SchedulerHttpResponse, securityLevel: SchedulerSecurityLevel) =
    inSchedulerThread {
      cppProxy.java_execute_http_with_security_level(request, response, securityLevel.cppName)
    }
}
